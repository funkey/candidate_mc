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
#include <features/FeatureExtractor.h>

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The treemc project file.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionRawImage(
		util::_long_name        = "rawImage",
		util::_short_name       = "r",
		util::_description_text = "The raw image.",
		util::_default_value    = "raw.tif");

util::ProgramOption optionResX(
		util::_long_name        = "resX",
		util::_description_text = "The x resolution of one pixel in the raw image.",
		util::_default_value    = 1);

util::ProgramOption optionResY(
		util::_long_name        = "resY",
		util::_description_text = "The y resolution of one pixel in the raw image.",
		util::_default_value    = 1);

util::ProgramOption optionResZ(
		util::_long_name        = "resZ",
		util::_description_text = "The z resolution of one pixel in the raw image.",
		util::_default_value    = 1);

util::ProgramOption optionOffsetX(
		util::_long_name        = "offsetX",
		util::_description_text = "The x offset of the raw image.",
		util::_default_value    = 0);

util::ProgramOption optionOffsetY(
		util::_long_name        = "offsetY",
		util::_description_text = "The y offset of the raw image.",
		util::_default_value    = 0);

util::ProgramOption optionOffsetZ(
		util::_long_name        = "offsetZ",
		util::_description_text = "The z offset of the raw image.",
		util::_default_value    = 0);

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		Hdf5CragStore cragStore(optionProjectFile.as<std::string>());

		Crag crag;
		cragStore.retrieveCrag(crag);

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		util::point<float, 3> resolution(
				optionResX,
				optionResY,
				optionResZ);
		util::point<float, 3> offset(
				optionOffsetX,
				optionOffsetY,
				optionOffsetZ);

		// get information about the image to read
		std::string filename = optionRawImage;
		vigra::ImageImportInfo info(filename.c_str());
		ExplicitVolume<float> raw(info.width(), info.height(), 1);
		importImage(info, raw.data().bind<2>(0));
		raw.setResolution(resolution);
		raw.setOffset(offset);

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

