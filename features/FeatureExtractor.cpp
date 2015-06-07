#include <fstream>
#include <sstream>
#include <region_features/RegionFeatures.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/helpers.hpp>
#include <util/timing.h>
#include "FeatureExtractor.h"
#include <vigra/flatmorphology.hxx>
#include <vigra/multi_morphology.hxx>

logger::LogChannel featureextractorlog("featureextractorlog", "[FeatureExtractor] ");

util::ProgramOption optionShapeFeatures(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "shapeFeatures",
	util::_description_text = "Compute shape morphology features for each candidate.",
	util::_default_value    = true
);

util::ProgramOption optionBoundariesFeatures(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "boundariesFeatures",
	util::_description_text = "Use the boundary image to compute image features for each candidate."
);

util::ProgramOption optionBoundariesBoundaryFeatures(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "boundariesBoundaryFeatures",
	util::_description_text = "Use the boundary image to compute image features for the boundary of each candidate."
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

util::ProgramOption optionMinMaxFromProject(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "minMaxFromProject",
	util::_description_text = "For the feature normalization, instead of using the min and max of the extracted features, "
	                          "use the min and max provided in the project file."
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
FeatureExtractor::extract() {

	NodeFeatures nodeFeatures(_crag);
	EdgeFeatures edgeFeatures(_crag);

	extractNodeFeatures(nodeFeatures);
	extractEdgeFeatures(nodeFeatures, edgeFeatures);

	_cragStore->saveNodeFeatures(_crag, nodeFeatures);
	_cragStore->saveEdgeFeatures(_crag, edgeFeatures);
}

void
FeatureExtractor::extractNodeFeatures(NodeFeatures& nodeFeatures) {

	LOG_USER(featureextractorlog) << "extracting node featuers" << std::endl;

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

		UTIL_TIME_SCOPE("extract node region features");

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
				_raw.data().subarray(
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

		if (nodeBoundingBox.depth() == 1) {

			RegionFeatures<2, float, unsigned char>::Parameters p;
			p.computeRegionprops = optionShapeFeatures;
			if (optionNoCoordinatesStatistics)
				p.statisticsParameters.computeCoordinateStatistics = false;
			p.regionpropsParameters.numAnglePoints = optionFeaturePointinessAnglePoints;
			p.regionpropsParameters.contourVecAsArcSegmentRatio = optionFeaturePointinessVectorLength;
			p.regionpropsParameters.numAngleHistBins = optionFeaturePointinessHistogramBins;
			RegionFeatures<2, float, unsigned char> regionFeatures(rawNodeImage.bind<2>(0), labelImage.bind<2>(0), p);

			if (_crag.id(n) == 0)
				LOG_DEBUG(featureextractorlog) << regionFeatures.getFeatureNames() << std::endl;

			regionFeatures.fill(adaptor);

		} else {

			RegionFeatures<3, float, unsigned char>::Parameters p;
			p.computeRegionprops = optionShapeFeatures;
			if (optionNoCoordinatesStatistics)
				p.statisticsParameters.computeCoordinateStatistics = false;
			p.regionpropsParameters.numAnglePoints = optionFeaturePointinessAnglePoints;
			p.regionpropsParameters.contourVecAsArcSegmentRatio = optionFeaturePointinessVectorLength;
			p.regionpropsParameters.numAngleHistBins = optionFeaturePointinessHistogramBins;
			RegionFeatures<3, float, unsigned char> regionFeatures(rawNodeImage, labelImage, p);

			if (_crag.id(n) == 0)
				LOG_DEBUG(featureextractorlog) << regionFeatures.getFeatureNames() << std::endl;

			regionFeatures.fill(adaptor);
		}

		if (optionBoundariesFeatures) {

			vigra::MultiArrayView<3, float> boundariesNodeImage =
					_boundaries.data().subarray(
							Shape(nodeBoundingBox.min().x(), nodeBoundingBox.min().y(), nodeBoundingBox.min().z()),
							Shape(nodeBoundingBox.max().x(), nodeBoundingBox.max().y(), nodeBoundingBox.max().z()));

			if (nodeBoundingBox.depth() == 1) {

				RegionFeatures<2, float, unsigned char>::Parameters p;
				if (optionNoCoordinatesStatistics)
					p.statisticsParameters.computeCoordinateStatistics = false;
				RegionFeatures<2, float, unsigned char> boundariesRegionFeatures(boundariesNodeImage.bind<2>(0), labelImage.bind<2>(0), p);
				boundariesRegionFeatures.fill(adaptor);

				if (_crag.id(n) == 0)
					LOG_DEBUG(featureextractorlog) << boundariesRegionFeatures.getFeatureNames() << std::endl;

			} else {

				RegionFeatures<3, float, unsigned char>::Parameters p;
				if (optionNoCoordinatesStatistics)
					p.statisticsParameters.computeCoordinateStatistics = false;
				RegionFeatures<3, float, unsigned char> boundariesRegionFeatures(boundariesNodeImage, labelImage, p);
				boundariesRegionFeatures.fill(adaptor);

				if (_crag.id(n) == 0)
					LOG_DEBUG(featureextractorlog) << boundariesRegionFeatures.getFeatureNames() << std::endl;
			}

			if (optionBoundariesBoundaryFeatures) {

				unsigned int width  = nodeBoundingBox.width();
				unsigned int height = nodeBoundingBox.height();
				unsigned int depth  = nodeBoundingBox.depth();

				// create the boundary image by erosion and substraction...
				vigra::MultiArray<3, unsigned char> erosionImage(labelImage.shape());
				if (depth == 1)
					vigra::discErosion(labelImage.bind<2>(0), erosionImage.bind<2>(0), 1);
				else
					vigra::multiBinaryErosion(labelImage, erosionImage, 1);
				vigra::MultiArray<3, unsigned char> boundaryImage(labelImage.shape());
				boundaryImage = labelImage;
				boundaryImage -= erosionImage;

				// ...and fix the border of the label image
				if (depth == 1) {

					for (unsigned int x = 0; x < width; x++) {

						boundaryImage(x, 0, 0)        |= labelImage(x, 0, 0);
						boundaryImage(x, height-1, 0) |= labelImage(x, height-1, 0);
					}

					for (unsigned int y = 1; y < height-1; y++) {

						boundaryImage(0, y, 0)       |= labelImage(0, y, 0);
						boundaryImage(width-1, y, 0) |= labelImage(width-1, y, 0);
					}

				} else {

					for (unsigned int z = 0; z < depth;  z++)
					for (unsigned int y = 0; y < height; y++)
					for (unsigned int x = 0; x < width;  x++)
						if (x == 0 || y == 0 || z == 0 || x == width - 1 || y == height -1 || z == depth - 1)
							boundaryImage(x, y, z) |= labelImage(x, y, z);
				}

				if (depth == 1) {

					RegionFeatures<2, float, unsigned char>::Parameters p;
					if (optionNoCoordinatesStatistics)
						p.statisticsParameters.computeCoordinateStatistics = false;
					RegionFeatures<2, float, unsigned char> boundaryFeatures(boundariesNodeImage.bind<2>(0), boundaryImage.bind<2>(0), p);
					boundaryFeatures.fill(adaptor);

					if (_crag.id(n) == 0)
						LOG_DEBUG(featureextractorlog) << boundaryFeatures.getFeatureNames() << std::endl;

				} else {

					RegionFeatures<3, float, unsigned char>::Parameters p;
					if (optionNoCoordinatesStatistics)
						p.statisticsParameters.computeCoordinateStatistics = false;
					RegionFeatures<3, float, unsigned char> boundaryFeatures(boundariesNodeImage, boundaryImage, p);
					boundaryFeatures.fill(adaptor);

					if (_crag.id(n) == 0)
						LOG_DEBUG(featureextractorlog) << boundaryFeatures.getFeatureNames() << std::endl;
				}
			}
		}
	}

	LOG_USER(featureextractorlog) << "extracted " << nodeFeatures.dims() << " features" << std::endl;

	///////////////////
	// NORMALIZATION //
	///////////////////

	if (optionNormalize) {

		LOG_USER(featureextractorlog) << "normalizing features" << std::endl;

		if (optionMinMaxFromProject) {

			LOG_USER(featureextractorlog)
					<< "reading feature minmax from project file"
					<< std::endl;

			std::vector<double> min, max;
			_cragStore->retrieveNodeFeaturesMinMax(min, max);

			LOG_ALL(featureextractorlog)
					<< "normalizing features with"
					<<  std::endl << min << std::endl
					<< "and"
					<<  std::endl << max << std::endl;

			nodeFeatures.normalize(min, max);

		} else {

			nodeFeatures.normalize();
			_cragStore->saveNodeFeaturesMinMax(
					nodeFeatures.getMin(),
					nodeFeatures.getMax());
		}
	}

	//////////////////////
	//// POSTPROCESSING //
	//////////////////////

	if (optionAddFeatureSquares || optionAddPairwiseFeatureProducts) {

		LOG_USER(featureextractorlog) << "adding feature products" << std::endl;

		for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

			std::vector<double>& features = nodeFeatures[n];

			if (optionAddPairwiseFeatureProducts) {

				// compute all products of all features and add them as well
				unsigned int numOriginalFeatures = features.size();
				for (unsigned int i = 0; i < numOriginalFeatures; i++)
					for (unsigned int j = i; j < numOriginalFeatures; j++)
						features.push_back(features[i]*features[j]);
			} else {

				// compute all squares of all features and add them as well
				unsigned int numOriginalFeatures = features.size();
				for (unsigned int i = 0; i < numOriginalFeatures; i++)
					features.push_back(features[i]*features[i]);
			}
		}
	}

	// append a 1 for bias
	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
		nodeFeatures.append(n, 1);

	LOG_USER(featureextractorlog)
			<< "after postprocessing, we have "
			<< nodeFeatures.dims()
			<< " features per node" << std::endl;

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
	int level          = 1; // level of leaf nodes

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
FeatureExtractor::extractEdgeFeatures(
		const NodeFeatures& nodeFeatures,
		EdgeFeatures&       edgeFeatures) {

	///////////////////
	// NORMALIZATION //
	///////////////////

	if (optionNormalize && edgeFeatures.dims() > 0) {

		LOG_USER(featureextractorlog) << "normalizing features" << std::endl;

		if (optionMinMaxFromProject) {

			LOG_USER(featureextractorlog)
					<< "reading feature minmax from project file"
					<< std::endl;

			std::vector<double> min, max;
			_cragStore->retrieveEdgeFeaturesMinMax(min, max);

			LOG_ALL(featureextractorlog)
					<< "normalizing features with"
					<<  std::endl << min << std::endl
					<< "and"
					<<  std::endl << max << std::endl;

			edgeFeatures.normalize(min, max);

		} else {

			edgeFeatures.normalize();
			_cragStore->saveEdgeFeaturesMinMax(
					edgeFeatures.getMin(),
					edgeFeatures.getMax());
		}
	}

	//////////////////////
	//// POSTPROCESSING //
	//////////////////////

	if (optionAddFeatureSquares || optionAddPairwiseFeatureProducts) {

		LOG_USER(featureextractorlog) << "adding feature products" << std::endl;

		for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e) {

			std::vector<double>& features = edgeFeatures[e];

			if (optionAddPairwiseFeatureProducts) {

				// compute all products of all features and add them as well
				unsigned int numOriginalFeatures = features.size();
				for (unsigned int i = 0; i < numOriginalFeatures; i++)
					for (unsigned int j = i; j < numOriginalFeatures; j++)
						features.push_back(features[i]*features[j]);
			} else {

				// compute all squares of all features and add them as well
				unsigned int numOriginalFeatures = features.size();
				for (unsigned int i = 0; i < numOriginalFeatures; i++)
					features.push_back(features[i]*features[i]);
			}
		}
	}

	// append a 1 for bias
	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e)
		edgeFeatures.append(e, 1);
}
