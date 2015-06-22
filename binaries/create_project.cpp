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
#include <crag/CragStackCombiner.h>
#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <io/volumes.h>
#include <vigra/impex.hxx>
#include <vigra/multi_labeling.hxx>

util::ProgramOption optionMergeTree(
		util::_long_name        = "mergeTree",
		util::_short_name       = "m",
		util::_description_text = "The merge-tree image. If this is a directory, one mergtree will be extracted "
		                          "per image in the directory and adjacencies introduced across subsequent images.");

util::ProgramOption optionSupervoxels(
		util::_long_name        = "supervoxels",
		util::_description_text = "A volume (single image or directory of images) with supervoxel ids. Use this together "
		                          "with mergeHistory.");

util::ProgramOption optionMergeHistory(
		util::_long_name        = "mergeHistory",
		util::_description_text = "A file containing lines 'a b c' to indicate that regions a and b merged into region c.");

util::ProgramOption optionIntensities(
		util::_long_name        = "intensities",
		util::_short_name       = "i",
		util::_description_text = "The raw intensity image or directory of images.",
		util::_default_value    = "raw.tif");

util::ProgramOption optionBoundaries(
		util::_long_name        = "boundaries",
		util::_short_name       = "b",
		util::_description_text = "The boundary prediciton image or directory of images.");

util::ProgramOption optionGroundTruth(
		util::_long_name        = "groundTruth",
		util::_short_name       = "g",
		util::_description_text = "An optional ground-truth image or directory of images.");

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

		util::point<float, 3> resolution(
				optionResX,
				optionResY,
				optionResZ);
		util::point<float, 3> offset(
				optionOffsetX,
				optionOffsetY,
				optionOffsetZ);

		Crag crag;

		if (optionMergeTree) {

			// get information about the image to read
			std::string mergeTreePath = optionMergeTree;

			if (boost::filesystem::is_directory(boost::filesystem::path(mergeTreePath))) {

				std::vector<std::string> files = getImageFiles(mergeTreePath);

				// process one image after another
				std::vector<Crag> crags(files.size());

				int i = 0;
				for (std::string file : files) {
					
					LOG_USER(logger::out) << "reading crag from " << file << std::endl;

					readCrag(file, crags[i], resolution, offset + util::point<float, 3>(0, 0, resolution.z()*i));
					i++;
				}

				// combine crags
				CragStackCombiner combiner;
				combiner.combine(crags, crag);

			} else {

				readCrag(mergeTreePath, crag, resolution, offset);
			}

		} else if (optionSupervoxels.as<bool>() && optionMergeHistory.as<bool>()) {

			readCrag(optionSupervoxels, optionMergeHistory, crag, resolution, offset);

		} else {

			LOG_ERROR(logger::out)
					<< "at least one of mergtree or (supervoxels && mergeHistory) "
					<< "have to be given to create a CRAG" << std::endl;
		}

		boost::filesystem::remove(optionProjectFile.as<std::string>());
		Hdf5CragStore store(optionProjectFile.as<std::string>());
		store.saveCrag(crag);

		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());

		ExplicitVolume<float> intensities = readVolume<float>(getImageFiles(optionIntensities));
		intensities.setResolution(resolution);
		intensities.setOffset(offset);
		intensities.normalize();
		volumeStore.saveIntensities(intensities);

		if (optionGroundTruth) {

			ExplicitVolume<int> groundTruth = readVolume<int>(getImageFiles(optionGroundTruth));

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

		if (optionBoundaries) {

			ExplicitVolume<float> boundaries = readVolume<float>(getImageFiles(optionBoundaries));
			boundaries.setResolution(resolution);
			boundaries.setOffset(offset);
			boundaries.normalize();
			volumeStore.saveBoundaries(boundaries);
		}

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}
