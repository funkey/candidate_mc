/**
 * Reads a treemc project file containing features and solves the segmentation 
 * problem for a given set of feature weights.
 */

#include <iostream>
#include <boost/filesystem.hpp>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <io/Hdf5CragStore.h>
#include <inference/MultiCut.h>

#include <vigra/functorexpression.hxx>

util::ProgramOption optionFeatureWeights(
		util::_long_name        = "featureWeights",
		util::_short_name       = "f",
		util::_description_text = "A file containing feature weights.",
		util::_default_value    = "feature_weights.txt");

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The treemc project file.",
		util::_default_value    = "project.hdf");

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		Hdf5CragStore cragStore(optionProjectFile.as<std::string>());

		Crag crag;
		cragStore.retrieveCrag(crag);

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		cragStore.retrieveNodeFeatures(crag, nodeFeatures);
		cragStore.retrieveEdgeFeatures(crag, edgeFeatures);

		//Crag crag;

		//Crag::Node a = crag.addNode();
		//Crag::Node b = crag.addNode();
		//Crag::Node c = crag.addNode();
		//Crag::Node d = crag.addNode();
		//Crag::Node e = crag.addNode();

		//Crag::Edge cd = crag.addAdjacencyEdge(c, d);
		//Crag::Edge de = crag.addAdjacencyEdge(d, e);
		//Crag::Edge be = crag.addAdjacencyEdge(b, e);
		//Crag::Edge ce = crag.addAdjacencyEdge(c, e);

		//crag.addSubsetArc(c, b);
		//crag.addSubsetArc(d, b);
		//crag.addSubsetArc(b, a);
		//crag.addSubsetArc(e, a);

		Crag::NodeMap<double> nodeWeights(crag);
		Crag::EdgeMap<double> edgeWeights(crag);

		//nodeWeights[a] = 0;
		//nodeWeights[b] = 0;
		//nodeWeights[c] = 0;
		//nodeWeights[d] = 0;
		//nodeWeights[e] = 0;

		//edgeWeights[cd] = -1;
		//edgeWeights[de] =  1;
		//edgeWeights[be] = -0.5;
		//edgeWeights[ce] = -0.5;

		for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
			nodeWeights[n] = 0;

		float bias = -3000;
		for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e) {

			float intensityU = nodeFeatures[crag.u(e)][4];
			float intensityV = nodeFeatures[crag.v(e)][4];

			edgeWeights[e] = bias + pow(intensityU - intensityV, 2);
		}

		MultiCut multicut(crag);

		multicut.setNodeWeights(nodeWeights);
		multicut.setEdgeWeights(edgeWeights);
		multicut.solve();

		//////////////////
		// IMAGE OUTPUT //
		//////////////////

		util::box<float, 3> cragBB = crag.getBoundingBox();
		util::point<float, 3> resolution;
		for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

			resolution = crag.getVolumes()[n].getResolution();
			break;
		}

		// create a vigra multi-array large enough to hold all volumes
		vigra::MultiArray<3, int> components(
				vigra::Shape3(
					cragBB.width() /resolution.x(),
					cragBB.height()/resolution.y(),
					cragBB.depth() /resolution.z()),
				std::numeric_limits<int>::max());

		for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

			if (crag.getVolumes()[n].getDiscreteBoundingBox().isZero())
				continue;

			const util::point<float, 3>&      volumeOffset     = crag.getVolumes()[n].getOffset();
			const util::box<unsigned int, 3>& volumeDiscreteBB = crag.getVolumes()[n].getDiscreteBoundingBox();

			util::point<unsigned int, 3> begin = (volumeOffset - cragBB.min())/resolution;
			util::point<unsigned int, 3> end   = begin +
					util::point<unsigned int, 3>(
							volumeDiscreteBB.width(),
							volumeDiscreteBB.height(),
							volumeDiscreteBB.depth());

			vigra::combineTwoMultiArrays(
					crag.getVolumes()[n].data(),
					components.subarray(
							vigra::Shape3(
									begin.x(),
									begin.y(),
									begin.z()),
							vigra::Shape3(
									end.x(),
									end.y(),
									end.z())),
					components.subarray(
							vigra::Shape3(
									begin.x(),
									begin.y(),
									begin.z()),
							vigra::Shape3(
									end.x(),
									end.y(),
									end.z())),
					vigra::functor::ifThenElse(
							vigra::functor::Arg1() == vigra::functor::Param(1),
							vigra::functor::Param(multicut.getComponents()[n]),
							vigra::functor::Arg2()
					));
		}

		vigra::exportImage(
				components.bind<2>(0),
				vigra::ImageExportInfo("solution.tif"));

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}



