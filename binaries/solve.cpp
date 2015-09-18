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
#include <util/helpers.hpp>
#include <util/timing.h>
#include <io/CragImport.h>
#include <io/Hdf5CragStore.h>
#include <io/vectors.h>
#include <io/volumes.h>
#include <crag/DownSampler.h>
#include <crag/PlanarAdjacencyAnnotator.h>
#include <features/FeatureExtractor.h>
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
		util::_description_text = "The treemc project file.");

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		Crag        crag;
		CragVolumes volumes(crag);

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		Hdf5CragStore cragStore(optionProjectFile.as<std::string>());
		cragStore.retrieveCrag(crag);
		cragStore.retrieveVolumes(volumes);

		cragStore.retrieveNodeFeatures(crag, nodeFeatures);
		cragStore.retrieveEdgeFeatures(crag, edgeFeatures);

		std::vector<double> weights = retrieveVector<double>(optionFeatureWeights);

		Costs costs(crag);

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
		{
			UTIL_TIME_SCOPE("solve candidate multi-cut");
			multicut.solve();
		}
		multicut.storeSolution(volumes, "solution.tif");
		multicut.storeSolution(volumes, "solution_boundary.tif", true);

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}

