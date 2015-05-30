/**
 * Reads a merge-tree image and creates a candidate region adjacency graph 
 * (CRAG), which is stored in an HDF5 file for further processing.
 */

#include <iostream>
#include <boost/filesystem.hpp>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <crag/Crag.h>
#include <crag/MergeTreeParser.h>
#include <crag/PlanarAdjacencyAnnotator.h>
#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <vigra/impex.hxx>
#include <vigra/multi_labeling.hxx>

util::ProgramOption optionMergeTree(
		util::_long_name        = "mergeTree",
		util::_short_name       = "m",
		util::_description_text = "The merge-tree image.",
		util::_default_value    = "merge_tree.tif");

util::ProgramOption optionIntensities(
		util::_long_name        = "intensities",
		util::_short_name       = "i",
		util::_description_text = "The raw intensity image.",
		util::_default_value    = "raw.tif");

util::ProgramOption optionGroundTruth(
		util::_long_name        = "groundTruth",
		util::_short_name       = "g",
		util::_description_text = "An optional ground-truth image.");

util::ProgramOption optionExtractGroundTruthLabels(
		util::_long_name        = "extractGroundTruthLabels",
		util::_description_text = "Indicate that the ground truth consists of a foreground/background labeling "
		                          "(dark/bright) and each 4-connected component of foreground represents one region.");

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The treemc project file.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionResX(
		util::_long_name        = "resX",
		util::_description_text = "The x resolution of one pixel in the merge-tree image.",
		util::_default_value    = 1);

util::ProgramOption optionResY(
		util::_long_name        = "resY",
		util::_description_text = "The y resolution of one pixel in the merge-tree image.",
		util::_default_value    = 1);

util::ProgramOption optionResZ(
		util::_long_name        = "resZ",
		util::_description_text = "The z resolution of one pixel in the merge-tree image.",
		util::_default_value    = 1);

util::ProgramOption optionOffsetX(
		util::_long_name        = "offsetX",
		util::_description_text = "The x offset of the merge-tree image.",
		util::_default_value    = 0);

util::ProgramOption optionOffsetY(
		util::_long_name        = "offsetY",
		util::_description_text = "The y offset of the merge-tree image.",
		util::_default_value    = 0);

util::ProgramOption optionOffsetZ(
		util::_long_name        = "offsetZ",
		util::_description_text = "The z offset of the merge-tree image.",
		util::_default_value    = 0);


int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		util::point<float, 3> resolution(
				optionResX,
				optionResY,
				optionResZ);
		util::point<float, 3> offset(
				optionOffsetX,
				optionOffsetY,
				optionOffsetZ);

		// get information about the image to read
		std::string filename = optionMergeTree;
		vigra::ImageImportInfo info(filename.c_str());
		Image mergeTree(info.width(), info.height());
		importImage(info, vigra::destImage(mergeTree));
		mergeTree.setResolution(resolution);
		mergeTree.setOffset(offset);

		MergeTreeParser parser(mergeTree);

		Crag crag;
		parser.getCrag(crag);

		PlanarAdjacencyAnnotator annotator(PlanarAdjacencyAnnotator::Direct);
		annotator.annotate(crag);

		boost::filesystem::remove(optionProjectFile.as<std::string>());
		Hdf5CragStore store(optionProjectFile.as<std::string>());
		store.saveCrag(crag);

		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());

		filename = optionIntensities.as<std::string>();
		info = vigra::ImageImportInfo(filename.c_str());
		ExplicitVolume<float> intensities(info.width(), info.height(), 1);
		importImage(info, intensities.data().bind<2>(0));
		intensities.setResolution(resolution);
		intensities.setOffset(offset);
		volumeStore.saveIntensities(intensities);

		if (optionGroundTruth) {

			std::string filename = optionGroundTruth;
			vigra::ImageImportInfo info(filename.c_str());
			ExplicitVolume<int> groundTruth(info.width(), info.height(), 1);
			importImage(info, groundTruth.data().bind<2>(0));

			if (optionExtractGroundTruthLabels) {

				vigra::MultiArray<3, int> tmp(groundTruth.data().shape());
				vigra::labelMultiArrayWithBackground(
						groundTruth.data(),
						tmp);
				groundTruth.data() = tmp;
			}

			groundTruth.setResolution(resolution);
			groundTruth.setOffset(offset);
			volumeStore.saveGroundTruth(groundTruth);
		}

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}
