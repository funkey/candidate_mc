#include <iostream>
#include <vigra/impex.hxx>
#include <vigra/multi_array.hxx>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <util/helpers.hpp>

util::ProgramOption optionSourceImages(
		util::_long_name        = "in",
		util::_short_name       = "i",
		util::_description_text = "The input images.");

util::ProgramOption optionTargetImage(
		util::_long_name        = "out",
		util::_short_name       = "o",
		util::_description_text = "The output images.");

util::ProgramOption optionAddSeperator(
		util::_long_name        = "seperator",
		util::_short_name       = "s",
		util::_description_text = "Add a seperator with the given value between each pair of input images. If -1 is supplied, "
		                          "the maximal value found in the image will be used as seperator value.");

util::ProgramOption optionSeperatorWidth(
		util::_long_name        = "seperatorWidth",
		util::_short_name       = "w",
		util::_description_text = "The width of the seperator. Defaults to 1.",
		util::_default_value    = 1);

util::ProgramOption optionLabelImages(
		util::_long_name        = "labelImages",
		util::_short_name       = "l",
		util::_description_text = "Assume that the images contain label ids. When combining, "
		                          "make sure the ids are still unique.");

void printUsage() {

	std::cout << std::endl;
	std::cout << "combine_images [-s|-s0] [-l] <image_1> ... <image_n> <out>" << std::endl;
	std::cout << std::endl;
	std::cout << "  -s  Put a seperating line between each pair of images." << std::endl;
	std::cout << "      The intensity of the line will be the maximal intensity " << std::endl;
	std::cout << "      found in any input image." << std::endl;
	std::cout << "  -s0 Put a seperating line between each pair of images." << std::endl;
	std::cout << "      The intensity of the line will be the 0." << std::endl;
	std::cout << "  -l  Assume that the images contain label ids. When combining," << std::endl;
	std::cout << "      make sure the ids are still unique." << std::endl;
}

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);

		std::istringstream iss(optionSourceImages.as<std::string>());
		std::vector<std::string> sources{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};

		std::string target = optionTargetImage.as<std::string>();
		bool addSeperator = optionAddSeperator;
		int seperatorWidth = optionSeperatorWidth.as<int>();
		float seperatorValue = 0;
		if (optionAddSeperator)
			seperatorValue = optionAddSeperator.as<float>();
		bool labelImages = optionLabelImages;

		std::cout << "in : " << sources << std::endl;
		std::cout << "out: " << target << std::endl;
		std::cout << "sep: " << addSeperator << std::endl;
		std::cout << "w  : " << seperatorWidth << std::endl;
		std::cout << "v  : " << seperatorValue << std::endl;
		std::cout << "l  : " << labelImages << std::endl;

		std::vector<vigra::MultiArray<2, float> > images;

		unsigned int width  = 0;
		unsigned int height = 0;
		std::string inputPixelType;

		float labelOffset = 0;
		for (std::string source : sources) {

			// get information about the image to read
			vigra::ImageImportInfo info(source.c_str());
			if (inputPixelType.empty())
				inputPixelType = info.getPixelType();

			// create new image
			images.push_back(vigra::MultiArray<2, float>(vigra::Shape2(info.width(), info.height())));

			// read image
			importImage(info, vigra::destImage(images.back()));

			if (labelImages) {

				// get the max value in this image
				float min, max;
				images.back().minmax(&min, &max);

				// add the current label offset
				for (int y = 0; y < info.height(); y++)
					for (int x = 0; x < info.width(); x++)
						if (images.back()(x, y) != 0)
							images.back()(x, y) += labelOffset;

				// increase the label offset
				labelOffset += max;
			}

			width += info.width();
			height = info.height();
		}

		if (addSeperator)
			width += seperatorWidth*(sources.size() - 1);

		vigra::MultiArray<2, float> combined(vigra::Shape2(width, height));

		if (seperatorValue < 0) {

			// get max intensity
			float maxIntensity = 0;
			for (unsigned int i = 0; i < images.size(); i++) {

				float _, max;
				images[i].minmax(&_, &max);
				maxIntensity = std::max(max, maxIntensity);
			}

			std::cout << "adding separators with intensity " << maxIntensity << std::endl;

			// intialize combined image with max intensity
			combined = maxIntensity;

		} else {

			combined = 0;
		}

		unsigned int offset = 0;
		for (unsigned int i = 0; i < images.size(); i++) {

			combined.subarray(
					vigra::Shape2(offset, 0),
					vigra::Shape2(offset + images[i].width(), height)) = images[i];

			offset += images[i].width() + (addSeperator ? seperatorWidth : 0);
		}

		std::string outputPixelType = inputPixelType;
		// workaround: bilevel images get scrumbled on export
		if (outputPixelType == "BILEVEL")
			outputPixelType = "FLOAT";

		float min, max;
		combined.minmax(&min, &max);
		std::cout << "range of combined image: " << min << " - " << max << std::endl;
		std::cout
				<< "input pixel type was " << inputPixelType
				<< ", saving with pixel type " << outputPixelType
				<< std::endl;

		vigra::exportImage(vigra::srcImageRange(combined), vigra::ImageExportInfo(target.c_str()).setPixelType(outputPixelType.c_str()));

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}
