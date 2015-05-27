/**
 * Reads a treemc project file containing features and solves the segmentation 
 * problem for a given set of feature weights.
 */

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <io/Hdf5CragStore.h>
#include <inference/MultiCut.h>

#include <vigra/functorexpression.hxx>

util::ProgramOption optionFeatureWeights(
		util::_long_name        = "featureWeights",
		util::_short_name       = "w",
		util::_description_text = "A file containing feature weights.",
		util::_default_value    = "feature_weights.txt");

util::ProgramOption optionForegroundBias(
		util::_long_name        = "foregroundBias",
		util::_short_name       = "f",
		util::_description_text = "A bias to be added to each node weight.",
		util::_default_value    = 0);

util::ProgramOption optionMergeBias(
		util::_long_name        = "mergeBias",
		util::_short_name       = "b",
		util::_description_text = "A bias to be added to each edge weight.",
		util::_default_value    = 0);

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

		std::ifstream weightsFile(optionFeatureWeights.as<std::string>());
		std::vector<double> weights;
		while (true) {
			double f;
			weightsFile >> f;
			if (!weightsFile.good())
				break;
			weights.push_back(f);
			std::cout << f << std::endl;
		}

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

		Crag::NodeMap<double> nodeCosts(crag);
		Crag::EdgeMap<double> edgeCosts(crag);

		//nodeCosts[a] = 0;
		//nodeCosts[b] = 0;
		//nodeCosts[c] = 0;
		//nodeCosts[d] = 0;
		//nodeCosts[e] = 0;

		//edgeCosts[cd] = -1;
		//edgeCosts[de] =  1;
		//edgeCosts[be] = -0.5;
		//edgeCosts[ce] = -0.5;

		float edgeBias = optionMergeBias;
		float nodeBias = optionForegroundBias;

		unsigned int numNodeFeatures = nodeFeatures.dims();
		unsigned int numEdgeFeatures = edgeFeatures.dims();

		for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

			nodeCosts[n] = nodeBias;

			for (unsigned int i = 0; i < numNodeFeatures; i++)
				nodeCosts[n] += weights[i]*nodeFeatures[n][i];
		}

		for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e) {

			edgeCosts[e] = edgeBias;

			for (unsigned int i = 0; i < numEdgeFeatures; i++)
				edgeCosts[e] += weights[i + numNodeFeatures]*edgeFeatures[e][i];
		}

		MultiCut multicut(crag);

		multicut.setNodeCosts(nodeCosts);
		multicut.setEdgeCosts(edgeCosts);
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
							vigra::functor::Param(multicut.getComponents()[n] + 1),
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



