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
#include <io/Hdf5CragStore.h>
#include <io/volumes.h>
#include <io/vectors.h>
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

util::ProgramOption optionMergeTree(
		util::_long_name        = "mergeTree",
		util::_short_name       = "m",
		util::_description_text = "The merge-tree image. If this is a directory, one mergtree will be extracted "
		                          "per image in the directory and adjacencies introduced across subsequent images.",
		util::_default_value    = "merge_tree.tif");

util::ProgramOption optionIntensities(
		util::_long_name        = "intensities",
		util::_short_name       = "i",
		util::_description_text = "The raw intensity image or directory of images.",
		util::_default_value    = "raw.tif");

util::ProgramOption optionBoundaries(
		util::_long_name        = "boundaries",
		util::_short_name       = "b",
		util::_description_text = "The boundary prediciton image or directory of images.",
		util::_default_value    = "prob.tif");

util::ProgramOption optionResX(
		util::_long_name        = "resX",
		util::_description_text = "The x resolution of one pixel in the input images.",
		util::_default_value    = 1);

util::ProgramOption optionResY(
		util::_long_name        = "resY",
		util::_description_text = "The y resolution of one pixel in the input images.",
		util::_default_value    = 1);

util::ProgramOption optionResZ(
		util::_long_name        = "resZ",
		util::_description_text = "The z resolution of one pixel in the input images.",
		util::_default_value    = 1);

util::ProgramOption optionOffsetX(
		util::_long_name        = "offsetX",
		util::_description_text = "The x offset of the input images.",
		util::_default_value    = 0);

util::ProgramOption optionOffsetY(
		util::_long_name        = "offsetY",
		util::_description_text = "The y offset of the input images.",
		util::_default_value    = 0);

util::ProgramOption optionOffsetZ(
		util::_long_name        = "offsetZ",
		util::_description_text = "The z offset of the input images.",
		util::_default_value    = 0);

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		Crag       crag;

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		if (optionProjectFile) {

			Hdf5CragStore cragStore(optionProjectFile.as<std::string>());
			cragStore.retrieveCrag(crag);

			cragStore.retrieveNodeFeatures(crag, nodeFeatures);
			cragStore.retrieveEdgeFeatures(crag, edgeFeatures);

		} else {

			util::point<float, 3> resolution(
					optionResX,
					optionResY,
					optionResZ);
			util::point<float, 3> offset(
					optionOffsetX,
					optionOffsetY,
					optionOffsetZ);

			std::string mergeTreePath = optionMergeTree;
			readCrag(mergeTreePath, crag, resolution, offset);

			ExplicitVolume<float> intensities = readVolume<float>(getImageFiles(optionIntensities));
			intensities.setResolution(resolution);
			intensities.setOffset(offset);
			intensities.normalize();

			ExplicitVolume<float> boundaries = readVolume<float>(getImageFiles(optionBoundaries));
			boundaries.setResolution(resolution);
			boundaries.setOffset(offset);
			boundaries.normalize();

			FeatureExtractor featureExtractor(crag, intensities, boundaries);
			featureExtractor.extract(nodeFeatures, edgeFeatures);
		}

		Crag::Node n = Crag::NodeIt(crag);
		std::cout << "node " << crag.id(n) << " " << nodeFeatures[n] << std::endl;

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
		multicut.solve();
		multicut.storeSolution("solution.tif");
		multicut.storeSolution("solution_boundary.tif", true);

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}



