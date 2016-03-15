/**
 * Reads a merge-tree image and creates a candidate region adjacency graph 
 * (CRAG), which is stored in an HDF5 file for further processing.
 */

#include <iostream>
#include <memory>
#include <boost/filesystem.hpp>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <util/timing.h>
#include <crag/Crag.h>
#include <crag/CragStackCombiner.h>
#include <crag/DownSampler.h>
#include <crag/PlanarAdjacencyAnnotator.h>
#include <features/FeatureWeights.h>
#include <io/CragImport.h>
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
		                          "with mergeHistory or candidateSegmentation.");

util::ProgramOption optionMergeHistory(
		util::_long_name        = "mergeHistory",
		util::_description_text = "A file containing lines 'a b c' to indicate that regions a and b merged into region c.");

util::ProgramOption optionCandidateSegmentation(
		util::_long_name        = "candidateSegmentation",
		util::_description_text = "A volume (single image or directory of images) with a segmentation (segment id per pixel). "
		                          "Candidates will be added to the CRAG for each segment. For that, supervoxels will be assigned "
		                          "to the segment with maximal overlap.");

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
		util::_description_text = "The candidate mc project file.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionImportTrainingResult(
		util::_long_name        = "importTrainingResult",
		util::_description_text = "If set to a project file, will import feature weights and feature min/max from "
		                          "this file. Use this to create a testing dataset.");

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

util::ProgramOption optionDownsampleCrag(
		util::_long_name        = "downSampleCrag",
		util::_description_text = "Reduce the number of candidates in the CRAG by removing candidates smaller than minCandidateSize, "
		                          "followed by contraction of single children with their parents.");

util::ProgramOption optionMinCandidateSize(
		util::_long_name        = "minCandidateSize",
		util::_description_text = "The minimal size for a candidate to keep it during downsampling (see downSampleCrag).",
		util::_default_value    = 100);

std::set<Crag::Node>
collectLeafNodes(const Crag& crag, Crag::Node n) {

	std::set<Crag::Node> leafNodes;

	if (crag.isLeafNode(n)) {

		leafNodes.insert(n);

	} else {

		for (Crag::SubsetInArcIt e(crag, crag.toSubset(n)); e != lemon::INVALID; ++e) {

			Crag::Node child = crag.toRag(crag.getSubsetGraph().source(e));
			std::set<Crag::Node> leafs = collectLeafNodes(crag, child);

			for (Crag::Node l : leafs)
				leafNodes.insert(l);
		}
	}

	return leafNodes;
}

int main(int argc, char** argv) {

	UTIL_TIME_SCOPE("main");

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

		Crag* crag = new Crag();
		CragVolumes* volumes = new CragVolumes(*crag);
		Costs* mergeCosts = new Costs(*crag);

		CragImport import;

		bool alreadyDownsampled = false;

		if (optionMergeTree) {

			UTIL_TIME_SCOPE("read CRAG from mergetree");

			// get information about the image to read
			std::string mergeTreePath = optionMergeTree;

			if (boost::filesystem::is_directory(boost::filesystem::path(mergeTreePath))) {

				std::vector<std::string> files = getImageFiles(mergeTreePath);

				// process one image after another
				std::vector<std::unique_ptr<Crag>> crags(files.size());
				std::vector<std::unique_ptr<CragVolumes>> cragsVolumes;
				for (auto& c : crags) {
					c = std::unique_ptr<Crag>(new Crag);
					cragsVolumes.push_back(std::unique_ptr<CragVolumes>(new CragVolumes(*c)));
				}

				int i = 0;
				for (std::string file : files) {
					
					LOG_USER(logger::out) << "reading crag from " << file << std::endl;

					import.readCrag(file, *crags[i], *cragsVolumes[i], resolution, offset + util::point<float, 3>(0, 0, resolution.z()*i));
					i++;
				}

				if (optionDownsampleCrag) {

					UTIL_TIME_SCOPE("downsample CRAG");

					DownSampler downSampler(optionMinCandidateSize.as<int>());

					std::vector<std::unique_ptr<Crag>> downSampledCrags(crags.size());
					std::vector<std::unique_ptr<CragVolumes>> downSampledVolumes(crags.size());

					for (int i = 0; i < crags.size(); i++) {

						downSampledCrags[i]   = std::unique_ptr<Crag>(new Crag());
						downSampledVolumes[i] = std::unique_ptr<CragVolumes>(new CragVolumes(*downSampledCrags[i]));

						downSampler.process(*crags[i], *cragsVolumes[i], *downSampledCrags[i], *downSampledVolumes[i]);
					}

					std::swap(cragsVolumes, downSampledVolumes);
					std::swap(crags, downSampledCrags);

					// prevent another downsampling on the candidates added by 
					// the combiner
					alreadyDownsampled = true;
				}

				// combine crags
				CragStackCombiner combiner;
				combiner.combine(crags, cragsVolumes, *crag, *volumes);

			} else {

				import.readCrag(mergeTreePath, *crag, *volumes, resolution, offset);
			}

		} else if (optionSupervoxels.as<bool>() && (optionMergeHistory.as<bool>() || optionCandidateSegmentation.as<bool>())) {

			UTIL_TIME_SCOPE("read CRAG from merge history");

			if (optionMergeHistory) {

				std::string mergeHistoryPath = optionMergeHistory;

				if (boost::filesystem::is_directory(boost::filesystem::path(mergeHistoryPath))) {

					// get all merge-history files
					std::vector<std::string> mhFiles;
					for (boost::filesystem::directory_iterator i(mergeHistoryPath); i != boost::filesystem::directory_iterator(); i++)
						if (!boost::filesystem::is_directory(*i) && (
							i->path().extension() == ".txt" ||
							i->path().extension() == ".dat"
						))
							mhFiles.push_back(i->path().native());
					std::sort(mhFiles.begin(), mhFiles.end());

					// get all supervoxel files
					std::vector<std::string> svFiles = getImageFiles(optionSupervoxels);

					// process one image after another
					std::vector<std::unique_ptr<Crag>> crags(mhFiles.size());
					std::vector<std::unique_ptr<CragVolumes>> cragsVolumes;
					for (auto& c : crags) {
						c = std::unique_ptr<Crag>(new Crag);
						cragsVolumes.push_back(std::unique_ptr<CragVolumes>(new CragVolumes(*c)));
					}

					for (int i = 0; i < mhFiles.size(); i++) {
						
						LOG_USER(logger::out) << "reading crag from supervoxel file " << svFiles[i] << " and merge history " << mhFiles[i] << std::endl;

						Costs mergeCosts(*crags[i]);
						import.readCragFromMergeHistory(svFiles[i], mhFiles[i], *crags[i], *cragsVolumes[i], resolution, offset + util::point<float, 3>(0, 0, resolution.z()*i), mergeCosts);
					}

					if (optionDownsampleCrag) {

						UTIL_TIME_SCOPE("downsample CRAG");

						DownSampler downSampler(optionMinCandidateSize.as<int>());

						std::vector<std::unique_ptr<Crag>> downSampledCrags(crags.size());
						std::vector<std::unique_ptr<CragVolumes>> downSampledVolumes(crags.size());

						for (int i = 0; i < crags.size(); i++) {

							downSampledCrags[i]   = std::unique_ptr<Crag>(new Crag());
							downSampledVolumes[i] = std::unique_ptr<CragVolumes>(new CragVolumes(*downSampledCrags[i]));

							downSampler.process(*crags[i], *cragsVolumes[i], *downSampledCrags[i], *downSampledVolumes[i]);
						}

						std::swap(cragsVolumes, downSampledVolumes);
						std::swap(crags, downSampledCrags);

						// prevent another downsampling on the candidates added by 
						// the combiner
						alreadyDownsampled = true;
					}

					// combine crags
					CragStackCombiner combiner;
					combiner.combine(crags, cragsVolumes, *crag, *volumes);

				} else {

					import.readCragFromMergeHistory(optionSupervoxels, optionMergeHistory, *crag, *volumes, resolution, offset, *mergeCosts);

				}

			} else
				import.readCragFromCandidateSegmentation(optionSupervoxels, optionCandidateSegmentation, *crag, *volumes, resolution, offset);

		} else {

			LOG_ERROR(logger::out)
					<< "at least one of mergtree or (supervoxels && mergeHistory) "
					<< "have to be given to create a CRAG" << std::endl;

			return 1;
		}

		if (optionDownsampleCrag && !alreadyDownsampled) {

			UTIL_TIME_SCOPE("downsample CRAG");

			Crag* downSampled = new Crag();
			CragVolumes* downSampledVolumes = new CragVolumes(*downSampled);

			DownSampler downSampler(optionMinCandidateSize.as<int>());
			downSampler.process(*crag, *volumes, *downSampled, *downSampledVolumes);

			delete crag;
			delete volumes;
			crag = downSampled;
			volumes = downSampledVolumes;
		}

		{
			UTIL_TIME_SCOPE("find CRAG adjacencies");

			PlanarAdjacencyAnnotator annotator(PlanarAdjacencyAnnotator::Direct);
			annotator.annotate(*crag, *volumes);
		}

		// Statistics

		int numNodes = 0;
		int numRootNodes = 0;
		double sumSubsetDepth = 0;
		int maxSubsetDepth = 0;
		int minSubsetDepth = 1e6;

		for (Crag::NodeIt n(*crag); n != lemon::INVALID; ++n) {

			if (crag->isRootNode(n)) {

				int depth = crag->getLevel(n);

				sumSubsetDepth += depth;
				minSubsetDepth = std::min(minSubsetDepth, depth);
				maxSubsetDepth = std::max(maxSubsetDepth, depth);
				numRootNodes++;
			}

			numNodes++;
		}

		int numAdjEdges = 0;
		for (Crag::EdgeIt e(*crag); e != lemon::INVALID; ++e)
			numAdjEdges++;
		int numSubEdges = 0;
		for (Crag::SubsetArcIt e(*crag); e != lemon::INVALID; ++e)
			numSubEdges++;

		LOG_USER(logger::out) << "created CRAG" << std::endl;
		LOG_USER(logger::out) << "\t# nodes          : " << numNodes << std::endl;
		LOG_USER(logger::out) << "\t# root nodes     : " << numRootNodes << std::endl;
		LOG_USER(logger::out) << "\t# adjacencies    : " << numAdjEdges << std::endl;
		LOG_USER(logger::out) << "\t# subset edges   : " << numSubEdges << std::endl;
		LOG_USER(logger::out) << "\tmax subset depth : " << maxSubsetDepth << std::endl;
		LOG_USER(logger::out) << "\tmin subset depth : " << minSubsetDepth << std::endl;
		LOG_USER(logger::out) << "\tmean subset depth: " << sumSubsetDepth/numRootNodes << std::endl;

		// Store CRAG and volumes

		boost::filesystem::remove(optionProjectFile.as<std::string>());
		Hdf5CragStore store(optionProjectFile.as<std::string>());

		{
			UTIL_TIME_SCOPE("saving CRAG");

			store.saveCrag(*crag);
			store.saveVolumes(*volumes);
			store.saveCosts(*crag, *mergeCosts, "merge-scores");
		}

		{
			UTIL_TIME_SCOPE("saving volumes");

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
		}

		delete crag;
		delete volumes;

		if (optionImportTrainingResult) {

			LOG_USER(logger::out)
							<< "importing training results from "
							<< optionImportTrainingResult.as<std::string>()
							<< std::endl;

			Hdf5CragStore trainingStore(optionImportTrainingResult.as<std::string>());

			FeatureWeights weights;
			FeatureWeights min;
			FeatureWeights max;

			trainingStore.retrieveFeatureWeights(weights);
			trainingStore.retrieveFeaturesMin(min);
			trainingStore.retrieveFeaturesMax(max);

			store.saveFeatureWeights(weights);
			store.saveFeaturesMin(min);
			store.saveFeaturesMax(max);
		}

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}
