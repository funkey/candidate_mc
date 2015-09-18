/**
 * merge_tree
 *
 * Creates a gray-level merge-tree image given a region boundary prediction 
 * image.
 */

#include <iostream>
#include <fstream>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include <util/exceptions.h>
#include <util/helpers.hpp>
#include <vigra/impex.hxx>
#include <vigra/multi_array.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra/flatmorphology.hxx>
#include <vigra/slic.hxx>
#include <vigra/multi_convolution.hxx>
#include <mergetree/IterativeRegionMerging.h>
#include <mergetree/MedianEdgeIntensity.h>
#include <mergetree/SmallFirst.h>
#include <mergetree/MultiplyMinRegionSize.h>
#include <mergetree/RandomPerturbation.h>

util::ProgramOption optionSourceImage(
		util::_long_name        = "source",
		util::_short_name       = "s",
		util::_description_text = "An image to compute the merge tree for.",
		util::_default_value    = "source.png");

util::ProgramOption optionMergeTreeImage(
		util::_long_name        = "mergeTreeImage",
		util::_short_name       = "m",
		util::_description_text = "An image representing the merge tree.",
		util::_default_value    = "mergetree.png");

util::ProgramOption optionSuperpixelImage(
		util::_long_name        = "superpixelImage",
		util::_description_text = "Create an image with the initial superpixels.");

util::ProgramOption optionSuperpixelWithBordersImage(
		util::_long_name        = "superpixelWithBordersImage",
		util::_description_text = "Create an image with the initial superpixels.");

util::ProgramOption optionSuperpixelsFirstId(
		util::_long_name        = "superpixelFirstId",
		util::_description_text = "Set the first id to be used for the superpixels created with superpixelImage or superpixelWithBordersImage.",
		util::_default_value    = 0);

util::ProgramOption optionRagFile(
		util::_long_name        = "ragFile",
		util::_description_text = "A file to write the region adjacency graph for the initial superpixels.");

util::ProgramOption optionMedian(
		util::_long_name        = "medianFilter",
		util::_description_text = "Median filter the (possibly smoothed) input image with a disc of the given size for superpixel seed extraction. "
		                          "This affects only the seeds, the superpixels will be extracted on the (possibly smoothed) input image.");

util::ProgramOption optionSmooth(
		util::_long_name        = "smooth",
		util::_description_text = "Smooth the input image with a Gaussian kernel of the given stddev.");

util::ProgramOption optionSlicSuperpixels(
		util::_long_name        = "slicSuperpixels",
		util::_description_text = "Use SLIC superpixels instead of watersheds to obtain initial regions.");

util::ProgramOption optionSlicIntensityScaling(
		util::_long_name        = "slicIntensityScaling",
		util::_description_text = "How to scale the image intensity for comparison to spatial distance. Default is 1.0.",
		util::_default_value    = 1.0);

util::ProgramOption optionSliceSize(
		util::_long_name        = "slicSize",
		util::_description_text = "An upper limit on the SLIC superpixel size. Default is 10.",
		util::_default_value    = 10);

util::ProgramOption optionMergeSmallRegionsFirst(
		util::_long_name        = "mergeSmallRegionsFirst",
		util::_description_text = "Merge small regions first. For parameters, see smallRegionThreshold1, smallRegionThreshold2, and intensityThreshold.");

util::ProgramOption optionRandomPerturbation(
		util::_long_name        = "randomPerturbation",
		util::_short_name       = "r",
		util::_description_text = "Randomly (normally distributed) perturb the edge scores for merging.");

using namespace logger;

vigra::MultiArray<2, float>
readImage(std::string filename) {

	// get information about the image to read
	vigra::ImageImportInfo info(filename.c_str());

	// abort if image is not grayscale
	if (!info.isGrayscale()) {

		UTIL_THROW_EXCEPTION(
				IOError,
				filename << " is not a gray-scale image!");
	}

	vigra::MultiArray<2, float> image(info.shape());

	// read image
	importImage(info, vigra::destImage(image));

	return image;
}

int main(int optionc, char** optionv) {

	using namespace vigra::functor;

	try {

		/********
		 * INIT *
		 ********/

		// init command line parser
		util::ProgramOptions::init(optionc, optionv);

		// init logger
		logger::LogManager::init();

		// read image
		vigra::MultiArray<2, float> image = readImage(optionSourceImage);

		// smooth
		if (optionSmooth)
			vigra::gaussianSmoothMultiArray(
					image,
					image,
					optionSmooth.as<double>());
		//vigra::exportImage(
				//image,
				//vigra::ImageExportInfo("smoothed.tif").setPixelType("FLOAT"));

		// generate seeds
		vigra::MultiArray<2, int> initialRegions(image.shape());

		if (optionMedian) {

			// normalize
			float min, max;
			image.minmax(&min, &max);
			vigra::transformImage(
					image,
					image,
					(Arg1() - Param(min))/(Param(max) - Param(min)));

			// discretize to [0,255]
			vigra::MultiArray<2, int> medianFiltered(image.shape());
			vigra::transformImage(
					image,
					medianFiltered,
					Arg1()*Param(255));

			// median filter
			vigra::discMedian(
					medianFiltered,
					medianFiltered,
					optionMedian.as<int>());

			//vigra::exportImage(
					//medianFiltered,
					//vigra::ImageExportInfo("median_filtered.tif").setPixelType("UINT8"));

			vigra::generateWatershedSeeds(
					medianFiltered,
					initialRegions,
					vigra::IndirectNeighborhood,
					vigra::SeedOptions().extendedMinima());

		} else {

			vigra::generateWatershedSeeds(
					image,
					initialRegions,
					vigra::IndirectNeighborhood,
					vigra::SeedOptions().extendedMinima());
		}

		//vigra::exportImage(
				//initialRegions,
				//vigra::ImageExportInfo("seeds.tif").setPixelType("FLOAT"));

		// perform watersheds or find SLIC superpixels

		if (optionSlicSuperpixels) {

			unsigned int maxLabel = vigra::slicSuperpixels(
					image,
					initialRegions,
					optionSlicIntensityScaling.as<double>(),
					optionSliceSize.as<double>());

			LOG_USER(logger::out) << "found " << maxLabel << " SLIC superpixels" << std::endl;

		} else {

			unsigned int maxLabel = vigra::watershedsMultiArray(
					image, /* non-median filtered, possibly smoothed */
					initialRegions,
					vigra::IndirectNeighborhood);

			LOG_USER(logger::out) << "found " << maxLabel << " watershed regions" << std::endl;

			// report highest used superpixel id
			LOG_USER(logger::out) << "next superpixel id: ";
			std::cout << (maxLabel + optionSuperpixelsFirstId.as<int>()) << std::endl;
		}

		if (optionSuperpixelImage || optionSuperpixelWithBordersImage) {

			vigra::MultiArray<2, int> initialRegionsExport(initialRegions.shape());
			vigra::transformImage(
					initialRegions,
					initialRegionsExport,
					// vigra starts counting sp with 1
					Arg1() - Param(1 - optionSuperpixelsFirstId.as<int>()));

			if (optionSuperpixelImage)
				vigra::exportImage(
						initialRegionsExport,
						vigra::ImageExportInfo(optionSuperpixelImage.as<std::string>().c_str()).setPixelType("FLOAT"));

			if (optionSuperpixelWithBordersImage) {

				vigra::MultiArray<2, int> initialRegionsWithBorders(image.shape());
				initialRegionsWithBorders = initialRegionsExport;

				for (unsigned int y = 0; y < initialRegions.height() - 1; y++)
					for (unsigned int x = 0; x < initialRegions.width() - 1; x++) {

						float value = initialRegions(x, y);
						float right = initialRegions(x+1, y);
						float down  = initialRegions(x, y+1);
						float diag  = initialRegions(x+1, y+1);

						if (value != right || value != down || value != diag)
							initialRegionsWithBorders(x, y) = 0;
					}

				vigra::exportImage(
						initialRegionsWithBorders,
						vigra::ImageExportInfo(optionSuperpixelWithBordersImage.as<std::string>().c_str()));
			}
		}

		// extract merge tree
		IterativeRegionMerging merging(initialRegions);

		MedianEdgeIntensity mei(image);

		// create the RAG description for the median edge intensities
		if (optionRagFile)
			merging.storeRag(optionRagFile.as<std::string>(), mei);

		if (optionMergeSmallRegionsFirst) {

			SmallFirst<MedianEdgeIntensity> scoringFunction(
					merging.getRag(),
					image,
					initialRegions,
					mei);

			if (optionRandomPerturbation) {

				RandomPerturbation<SmallFirst<MedianEdgeIntensity> > rp(scoringFunction);
				merging.createMergeTree(rp);

			} else {

				merging.createMergeTree(scoringFunction);
			}

		} else {

			MultiplyMinRegionSize<MedianEdgeIntensity> scoringFunction(
					merging.getRag(),
					image,
					initialRegions,
					mei);

			if (optionRandomPerturbation) {

				RandomPerturbation<MultiplyMinRegionSize<MedianEdgeIntensity> > rp(scoringFunction);
				merging.createMergeTree(rp);

			} else {

				merging.createMergeTree(scoringFunction);
			}
		}

		LOG_USER(logger::out) << "writing merge tree..." << std::endl;

		vigra::exportImage(
				merging.getMergeTree(),
				vigra::ImageExportInfo(optionMergeTreeImage.as<std::string>().c_str()).setPixelType("FLOAT"));

	} catch (Exception& e) {

		handleException(e, std::cerr);
	}
}

