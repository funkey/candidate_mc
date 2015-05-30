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

		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());
		ExplicitVolume<float> raw;
		volumeStore.retrieveIntensities(raw);

		FeatureExtractor featureExtractor(crag, raw);
		featureExtractor.extract(
				nodeFeatures,
				edgeFeatures);

		cragStore.saveNodeFeatures(crag, nodeFeatures);
		cragStore.saveEdgeFeatures(crag, edgeFeatures);

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

