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
#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <io/SolutionImageWriter.h>
#include <features/FeatureExtractor.h>
#include <inference/CragSolverFactory.h>

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
		util::_description_text = "The candidate mc project file.");

util::ProgramOption optionExportSolution(
		util::_long_name        = "exportSolution",
		util::_description_text = "Create a volume export for the solution.");

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

	UTIL_TIME_SCOPE("main");

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

		cragStore.saveCosts(crag, costs, "costs");

		CragSolution solution(crag);
		std::unique_ptr<CragSolver> solver(CragSolverFactory::createSolver(crag, volumes));

		solver->setCosts(costs);
		{
			UTIL_TIME_SCOPE("solve candidate multi-cut");
			solver->solve(solution);
		}

		cragStore.saveSolution(crag, solution, "solution");

		if (optionExportSolution) {

			Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());
			ExplicitVolume<float> intensities;
			volumeStore.retrieveIntensities(intensities);

			SolutionImageWriter imageWriter;
			imageWriter.setExportArea(intensities.getBoundingBox());
			imageWriter.write(crag, volumes, solution, optionExportSolution.as<std::string>());
			imageWriter.write(crag, volumes, solution, optionExportSolution.as<std::string>() + "_boundary", true);
		}

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}

