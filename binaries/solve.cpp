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
#include <util/assert.h>
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

inline double dot(const std::vector<double>& a, const std::vector<double>& b) {

	UTIL_ASSERT_REL(a.size(), ==, b.size());

	double sum = 0;
	auto ba = a.begin();
	auto ea = a.end();
	auto bb = b.begin();

	while (ba != ea) {

		sum += (*ba)*(*bb);
		ba++;
		bb++;
	}

	return sum;
}

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

		FeatureWeights weights;
		cragStore.retrieveFeatureWeights(weights);

		Costs costs(crag);

		float edgeBias = optionMergeBias;
		float nodeBias = optionForegroundBias;

		for (Crag::CragNode n : crag.nodes()) {

			costs.node[n] = nodeBias;
			costs.node[n] += dot(weights[crag.type(n)], nodeFeatures[n]);
		}

		for (Crag::CragEdge e : crag.edges()) {

			costs.edge[e] = edgeBias;
			costs.edge[e] += dot(weights[crag.type(e)], edgeFeatures[e]);
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

