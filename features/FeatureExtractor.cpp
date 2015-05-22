#include <fstream>
#include <sstream>
#include <vigra/flatmorphology.hxx>
#include <region_features/RegionFeatures.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/helpers.hpp>
#include "FeatureExtractor.h"

logger::LogChannel featureextractorlog("featureextractorlog", "[FeatureExtractor] ");

util::ProgramOption optionShapeFeatures(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "shapeFeatures",
	util::_description_text = "Compute shape morphology features for each candidate.",
	util::_default_value    = true
);

util::ProgramOption optionProbabilityImageFeatures(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "probabilityImageFeatures",
	util::_description_text = "Use the probability image to compute image features for each candidate."
);

util::ProgramOption optionProbabilityImageBoundaryFeatures(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "probabilityImageBoundaryFeatures",
	util::_description_text = "Use the probability image to compute image features for the boundary of each candidate."
);

util::ProgramOption optionNoCoordinatesStatistics(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "noCoordinatesStatistics",
	util::_description_text = "Do not include statistics features that compute coordinates."
);

util::ProgramOption optionAddPairwiseFeatureProducts(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "addPairwiseProducts",
	util::_description_text = "For each pair of features f_i and f_j, add the product f_i*f_j to the feature vector as well."
);

util::ProgramOption optionAddFeatureSquares(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "addSquares",
	util::_description_text = "For each features f_i add the square f_i*f_i to the feature vector as well (implied by addPairwiseFeatureProducts)."
);

util::ProgramOption optionNormalize(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "normalize",
	util::_description_text = "Normalize each original feature to be in the interval [0,1]."
);

util::ProgramOption optionFeatureMinMaxFile(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "minMaxFile",
	util::_description_text = "For the feature normalization, instead of using the min and max of the extracted features, """""
	                          "use the min and max provided in the given file (first row min, second row max)."
);

util::ProgramOption optionFeaturePointinessAnglePoints(
	util::_module           = "candidate_mc.features.pointiness",
	util::_long_name        = "numAnglePoints",
	util::_description_text = "The number of points to sample equidistantly on the contour of nodes. Default is 50.",
	util::_default_value    = 50
);

util::ProgramOption optionFeaturePointinessVectorLength(
	util::_module           = "candidate_mc.features.pointiness",
	util::_long_name        = "angleVectorLength",
	util::_description_text = "The amount to walk on the contour from a sample point in either direction, to estimate the angle. Values are between "
	                          "0 (at the sample point) and 1 (at the next sample point). Default is 0.1.",
	util::_default_value    = 0.1
);

util::ProgramOption optionFeaturePointinessHistogramBins(
	util::_module           = "candidate_mc.features.pointiness",
	util::_long_name        = "numHistogramBins",
	util::_description_text = "The number of histogram bins for the measured angles. Default is 16.",
	util::_default_value    = 16
);

void
FeatureExtractor::extract(NodeFeatures& nodeFeatures, EdgeFeatures& edgeFeatures) {

	extractNodeFeatures(nodeFeatures);
	extractEdgeFeatures(edgeFeatures);
}

void
FeatureExtractor::extractNodeFeatures(NodeFeatures& nodeFeatures) {

	//////////////////////////
	// TOPOLOGICAL FEATURES //
	//////////////////////////

	for (Crag::SubsetNodeIt n(_crag); n != lemon::INVALID; ++n) {

		int numParents = 0;
		for (Crag::SubsetOutArcIt e(_crag, n); e != lemon::INVALID; ++e)
			numParents++;

		if (numParents == 0)
			extractTopologicalFeatures(
					nodeFeatures,
					n);
	}

	/////////////////////
	// REGION FEATURES //
	/////////////////////

	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

		LOG_ALL(featureextractorlog)
				<< "extracting features for node "
				<< _crag.id(n) << std::endl;

		// the bounding box of the volume
		const util::box<float, 3>& nodeBoundingBox = getBoundingBox(n);

		LOG_ALL(featureextractorlog)
				<< "node volume bounding box is "
				<< nodeBoundingBox << std::endl;

		// a view to the raw image for the node bounding box
		typedef vigra::MultiArrayView<3, float>::difference_type Shape;
		vigra::MultiArrayView<3, float> rawNodeImage =
				_data.data().subarray(
						Shape(nodeBoundingBox.min().x(), nodeBoundingBox.min().y(), nodeBoundingBox.min().z()),
						Shape(nodeBoundingBox.max().x(), nodeBoundingBox.max().y(), nodeBoundingBox.max().z()));

		LOG_ALL(featureextractorlog)
				<< "raw image has size "
				<< rawNodeImage.shape()
				<< std::endl;

		// the "label" image
		vigra::MultiArray<3, unsigned char> labelImage(
				vigra::Shape3(
						nodeBoundingBox.width(),
						nodeBoundingBox.height(),
						nodeBoundingBox.depth()));
		recFill(nodeBoundingBox, labelImage, n);

		LOG_ALL(featureextractorlog)
				<< "label image has size "
				<< labelImage.shape()
				<< std::endl;

		// an adaptor to access the feature map with the right node
		FeatureNodeAdaptor adaptor(n, nodeFeatures);

		RegionFeatures<3, float, unsigned char>::Parameters p;
		p.computeRegionprops = optionShapeFeatures;
		if (optionNoCoordinatesStatistics)
			p.statisticsParameters.computeCoordinateStatistics = false;
		p.regionpropsParameters.numAnglePoints = optionFeaturePointinessAnglePoints;
		p.regionpropsParameters.contourVecAsArcSegmentRatio = optionFeaturePointinessVectorLength;
		p.regionpropsParameters.numAngleHistBins = optionFeaturePointinessHistogramBins;
		RegionFeatures<3, float, unsigned char> regionFeatures(rawNodeImage, labelImage, p);

		regionFeatures.fill(adaptor);

		for (std::string name : regionFeatures.getFeatureNames())
			std::cout << name << std::endl;
	}

	///////////////////
	// NORMALIZATION //
	///////////////////

	//if (optionNormalize) {

		//LOG_USER(featureextractorlog) << "normalizing features" << std::endl;

		//if (optionFeatureMinMaxFile) {

			//LOG_USER(featureextractorlog)
					//<< "reading feature minmax from "
					//<< optionFeatureMinMaxFile.as<std::string>()
					//<< std::endl;

			//std::vector<double> min, max;

			//std::ifstream minMaxFile(optionFeatureMinMaxFile.as<std::string>().c_str());
			//std::string line;

			//if (!minMaxFile.good())
				//UTIL_THROW_EXCEPTION(
						//IOError,
						//"unable to open " << optionFeatureMinMaxFile.as<std::string>());

			//// min
			//{
				//std::getline(minMaxFile, line);
				//std::stringstream lineStream(line);

				//while (lineStream.good()) {

					//double f;

					//lineStream >> f;
					//if (lineStream.good())
						//min.push_back(f);
				//}
			//}

			//// max
			//{
				//std::getline(minMaxFile, line);
				//std::stringstream lineStream(line);

				//while (lineStream.good()) {

					//double f;

					//lineStream >> f;
					//if (lineStream.good())
						//max.push_back(f);
				//}
			//}

			//LOG_ALL(featureextractorlog)
					//<< "normalizing features with"
					//<<  std::endl << min << std::endl
					//<< "and"
					//<<  std::endl << max << std::endl;



			//_features->normalize(min, max);

		//} else {

			//_features->normalize();
		//}
	//}

	//////////////////////
	//// POSTPROCESSING //
	//////////////////////

	//if (optionAddFeatureSquares || optionAddPairwiseFeatureProducts) {

		//LOG_USER(featureextractorlog) << "adding feature products" << std::endl;

		//foreach (boost::shared_ptr<Node> node, *_nodes) {

			//std::vector<double>& features = _features->getFeatures(node->getId());

			//if (optionAddPairwiseFeatureProducts) {

				//// compute all products of all features and add them as well
				//unsigned int numOriginalFeatures = features.size();
				//for (unsigned int i = 0; i < numOriginalFeatures; i++)
					//for (unsigned int j = i; j < numOriginalFeatures; j++)
						//features.push_back(features[i]*features[j]);
			//} else {

				//// compute all squares of all features and add them as well
				//unsigned int numOriginalFeatures = features.size();
				//for (unsigned int i = 0; i < numOriginalFeatures; i++)
					//features.push_back(features[i]*features[i]);
			//}
		//}
	//}

	//// append a 1 for bias
	//foreach (boost::shared_ptr<Node> node, *_nodes)
		//_features->append(node->getId(), 1);

	//LOG_USER(featureextractorlog)
			//<< "after postprocessing, we have "
			//<< _features->getFeatures((*_nodes->begin())->getId()).size()
			//<< " features" << std::endl;

	LOG_USER(featureextractorlog) << "done" << std::endl;
}

util::box<float, 3>
FeatureExtractor::getBoundingBox(Crag::Node n) {

	util::box<float, 3> bb;

	int numChildren = 0;
	for (Crag::SubsetInArcIt e(_crag, _crag.toSubset(n)); e != lemon::INVALID; ++e) {

		bb += getBoundingBox(_crag.toRag(_crag.getSubsetGraph().oppositeNode(_crag.toSubset(n), e)));
		numChildren++;
	}

	if (numChildren == 0) {

		// leaf node
		return _crag.getVolumes()[n].getBoundingBox()/
			   _crag.getVolumes()[n].getResolution();
	}

	return bb;
}

void
FeatureExtractor::recFill(
		const util::box<float, 3>&           boundingBox,
		vigra::MultiArray<3, unsigned char>& labelImage,
		Crag::Node                           n) {

	int numChildren = 0;
	for (Crag::SubsetInArcIt e(_crag, _crag.toSubset(n)); e != lemon::INVALID; ++e) {

		recFill(boundingBox, labelImage, _crag.toRag(_crag.getSubsetGraph().oppositeNode(_crag.toSubset(n), e)));
		numChildren++;
	}

	if (numChildren == 0) {

		const ExplicitVolume<unsigned char>& volume = _crag.getVolumes()[n];

		util::point<unsigned int, 3> labelImageOffset = boundingBox.min();
		util::point<unsigned int, 3> nodeVolumeOffset =
				volume.getBoundingBox().min()/
				volume.getResolution();
		util::point<unsigned int, 3> offset = nodeVolumeOffset - labelImageOffset;

		for (unsigned int z = 0; z < volume.depth();  z++)
		for (unsigned int y = 0; y < volume.height(); y++)
		for (unsigned int x = 0; x < volume.width();  x++) {

			if (volume(x, y, z) > 0)
				labelImage(
						offset.x() + x,
						offset.y() + y,
						offset.z() + z) = volume(x, y, z);
		}
	}
}

std::pair<int, int>
FeatureExtractor::extractTopologicalFeatures(NodeFeatures& nodeFeatures, Crag::SubsetNode n) {

	int numChildren    = 0;
	int numDescendants = 0;
	int level          = 0;

	for (Crag::SubsetInArcIt e(_crag, n); e != lemon::INVALID; ++e) {

		std::pair<int, int> level_descendants =
				extractTopologicalFeatures(
						nodeFeatures,
						_crag.getSubsetGraph().oppositeNode(n, e));

		level = std::max(level, level_descendants.first + 1);
		numDescendants += level_descendants.second;
		numChildren++;
	}

	numDescendants += numChildren;

	nodeFeatures.append(_crag.toRag(n), level);
	nodeFeatures.append(_crag.toRag(n), numDescendants);

	return std::make_pair(level, numDescendants);
}

void
FeatureExtractor::extractEdgeFeatures(EdgeFeatures& /*edgeFeatures*/) {
}
