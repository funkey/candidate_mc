/**
 * Reads a treemc project file, computes features for each region and edge, and 
 * stores them in the project file.
 */

#include <iostream>
#include <boost/filesystem.hpp>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <features/FeatureExtractor.h>
#include <features/SkeletonExtractor.h>

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

		FeatureExtractor featureExtractor(crag, raw, boundaries);
		featureExtractor.setSampleSegmentations(sampleSegmentations);
		featureExtractor.extract(nodeFeatures, edgeFeatures);

		cragStore.saveNodeFeatures(crag, nodeFeatures);
		cragStore.saveEdgeFeatures(crag, edgeFeatures);

		Skeletons skeletons(crag);

		SkeletonExtractor skeletonExtractor(crag);
		skeletonExtractor.extract(skeletons);

		cragStore.saveSkeletons(crag, skeletons);

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

