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
#include <features/VolumeRays.h>
#include <learning/RandLoss.h>
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

util::ProgramOption optionMinMaxFromProject(
		util::_long_name        = "minMaxFromProject",
		util::_description_text = "Instead of computing the min and max values of the features for normalization, "
		                          "use min and max stored in the project file.");

util::ProgramOption optionNoSkeletons(
		util::_long_name        = "noSkeletons",
		util::_description_text = "Do not extract skeletons for the candidates.");

util::ProgramOption optionNoVolumeRays(
		util::_long_name        = "noVolumeRays",
		util::_description_text = "Do not extract rays locally describing the volume for the candidates.");

util::ProgramOption optionVolumeRaysSampleRadius(
		util::_long_name        = "volumeRaysSampleRadius",
		util::_description_text = "The size of the sphere to use to estimate the surface normal of boundary points.",
		util::_default_value    = 50);

util::ProgramOption optionVolumeRaysSampleDensity(
		util::_long_name        = "volumeRaysSampleDensity",
		util::_description_text = "Distance between sample points in the normal estimation sphere.",
		util::_default_value    = 2);

int main(int argc, char** argv) {

	UTIL_TIME_SCOPE("main");

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		Hdf5CragStore cragStore(optionProjectFile.as<std::string>());

		LOG_USER(logger::out) << "reading CRAG and volumes" << std::endl;

		Crag        crag;
		CragVolumes volumes(crag);
		cragStore.retrieveCrag(crag);
		cragStore.retrieveVolumes(volumes);

		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());
		ExplicitVolume<float> raw;
		ExplicitVolume<float> boundaries;
		volumeStore.retrieveIntensities(raw);
		volumeStore.retrieveBoundaries(boundaries);

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		if (!optionNoFeatures) {

			LOG_USER(logger::out) << "extracting features" << std::endl;

			FeatureWeights min, max;

			if (optionMinMaxFromProject) {

				cragStore.retrieveFeaturesMin(min);
				cragStore.retrieveFeaturesMax(max);
			}

			FeatureExtractor featureExtractor(crag, volumes, raw, boundaries);
			featureExtractor.extract(nodeFeatures, edgeFeatures, min, max);

			if (!optionMinMaxFromProject) {

				cragStore.saveFeaturesMin(min);
				cragStore.saveFeaturesMax(max);
			}
		}

		if (optionAppendBestEffortFeature) {

			ExplicitVolume<int> groundTruth;
			volumeStore.retrieveGroundTruth(groundTruth);

			RandLoss randLoss(crag, volumes, groundTruth);
			BestEffort bestEffort(crag, volumes, randLoss);

			for (Crag::CragNode n : crag.nodes())
				if (bestEffort.selected(n))
					nodeFeatures.append(n, 1);
				else
					nodeFeatures.append(n, 0);

			for (Crag::CragEdge e : crag.edges())
				if (bestEffort.selected(e))
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

		if (!optionNoVolumeRays) {

			VolumeRays rays(crag);

			{
				UTIL_TIME_SCOPE("extracting volume rays");
				rays.extractFromVolumes(volumes, optionVolumeRaysSampleRadius, optionVolumeRaysSampleDensity);
			}

			{
				UTIL_TIME_SCOPE("storing volume rays");
				cragStore.saveVolumeRays(rays);
			}
		}

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

