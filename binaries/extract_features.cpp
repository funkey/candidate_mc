/**
 * Reads a treemc project file, computes features for each region and edge, and 
 * stores them in the project file.
 */

#include <iostream>
#include <boost/filesystem.hpp>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <util/timing.h>
#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <features/FeatureExtractor.h>
#include <features/SkeletonExtractor.h>
#include <learning/OverlapLoss.h>
#include <learning/BestEffort.h>

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The treemc project file.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionAppendBestEffortFeature(
		util::_long_name        = "appendBestEffortFeature",
		util::_description_text = "Compute the best-effort from ground-truth and append a binary feature "
		                          "to each node and edge indicating if this node or edge is part of the "
		                          "best-effort solution. Used for testing the learning method.");

util::ProgramOption optionNoFeatures(
		util::_long_name        = "noFeatures",
		util::_description_text = "Perform a dry run and don't extract any features (except for "
		                          "best-effort features, if set). Used for testing the learning "
		                          "method.");

util::ProgramOption optionNoSkeletons(
		util::_long_name        = "noSkeletons",
		util::_description_text = "Do not extract skeletons for the candidates.");

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		Hdf5CragStore cragStore(optionProjectFile.as<std::string>());

		Crag        crag;
		CragVolumes volumes(crag);
		cragStore.retrieveCrag(crag);
		cragStore.retrieveVolumes(volumes);

		std::vector<std::vector<std::set<Crag::Node>>> sampleSegmentations;
		std::vector<std::string> segmentationNames = cragStore.getSegmentationNames();
		for (std::string name : segmentationNames) {

			std::vector<std::set<Crag::Node>> segmentation;
			cragStore.retrieveSegmentation(crag, segmentation, name);
			sampleSegmentations.push_back(segmentation);
		}

		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());
		ExplicitVolume<float> raw;
		ExplicitVolume<float> boundaries;
		volumeStore.retrieveIntensities(raw);
		volumeStore.retrieveBoundaries(boundaries);

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		if (!optionNoFeatures) {

			FeatureExtractor featureExtractor(crag, volumes, raw, boundaries);
			featureExtractor.setSampleSegmentations(sampleSegmentations);
			featureExtractor.extract(nodeFeatures, edgeFeatures);
		}

		if (optionAppendBestEffortFeature) {

			ExplicitVolume<int> groundTruth;
			volumeStore.retrieveGroundTruth(groundTruth);

			OverlapLoss overlapLoss(crag, volumes, groundTruth);
			BestEffort bestEffort(crag, volumes, overlapLoss);

			for (Crag::CragNode n : crag.nodes())
				if (bestEffort.node[n])
					nodeFeatures.append(n, 1);
				else
					nodeFeatures.append(n, 0);

			for (Crag::CragEdge e : crag.edges())
				if (bestEffort.edge[e])
					edgeFeatures.append(e, 1);
				else
					edgeFeatures.append(e, 0);
		}

		{
			UTIL_TIME_SCOPE("storing features");
			cragStore.saveNodeFeatures(crag, nodeFeatures);
			cragStore.saveEdgeFeatures(crag, edgeFeatures);
		}

		if (!optionNoSkeletons) {

			Skeletons skeletons(crag);

			SkeletonExtractor skeletonExtractor(crag, volumes);
			skeletonExtractor.extract(skeletons);

			{
				UTIL_TIME_SCOPE("storing skeletons");
				cragStore.saveSkeletons(crag, skeletons);
			}
		}

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

