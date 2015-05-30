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

		Costs costs(crag);

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

			costs.node[n] = nodeBias;

			for (unsigned int i = 0; i < numNodeFeatures; i++)
				costs.node[n] += weights[i]*nodeFeatures[n][i];
		}

		for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e) {

			costs.edge[e] = edgeBias;

			for (unsigned int i = 0; i < numEdgeFeatures; i++)
				costs.edge[e] += weights[i + numNodeFeatures]*edgeFeatures[e][i];
		}

		MultiCut multicut(crag);

		multicut.setCosts(costs);
		multicut.solve();
		multicut.storeSolution("solution.tif");

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}



