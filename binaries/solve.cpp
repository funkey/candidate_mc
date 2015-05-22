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

		//Hdf5CragStore cragStore(optionProjectFile.as<std::string>());

		//Crag crag;
		//cragStore.retrieveCrag(crag);

		//NodeFeatures nodeFeatures(crag);
		//EdgeFeatures edgeFeatures(crag);

		//cragStore.retrieveNodeFeatures(crag, nodeFeatures);
		//cragStore.retrieveEdgeFeatures(crag, edgeFeatures);

		Crag crag;

		Crag::Node a = crag.addNode();
		Crag::Node b = crag.addNode();
		Crag::Node c = crag.addNode();
		Crag::Node d = crag.addNode();
		Crag::Node e = crag.addNode();

		Crag::Edge cd = crag.addAdjacencyEdge(c, d);
		Crag::Edge de = crag.addAdjacencyEdge(d, e);
		Crag::Edge be = crag.addAdjacencyEdge(b, e);
		Crag::Edge ce = crag.addAdjacencyEdge(c, e);

		crag.addSubsetArc(c, b);
		crag.addSubsetArc(d, b);
		crag.addSubsetArc(b, a);
		crag.addSubsetArc(e, a);

		Crag::NodeMap<double> nodeWeights(crag);
		Crag::EdgeMap<double> edgeWeights(crag);

		nodeWeights[a] = 0;
		nodeWeights[b] = 0;
		nodeWeights[c] = 0;
		nodeWeights[d] = 0;
		nodeWeights[e] = 0;

		edgeWeights[cd] = -1;
		edgeWeights[de] =  1;
		edgeWeights[be] = -0.5;
		edgeWeights[ce] = -0.5;

		MultiCut multicut(crag);

		multicut.setNodeWeights(nodeWeights);
		multicut.setEdgeWeights(edgeWeights);
		multicut.solve();

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}



