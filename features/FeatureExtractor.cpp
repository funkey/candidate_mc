#include <fstream>
#include <sstream>
#include <region_features/RegionFeatures.h>
#include <io/vectors.h>
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

util::ProgramOption optionMinMaxFromFiles(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "minMaxFromFiles",
	util::_description_text = "For the feature normalization, instead of using the min and max of the extracted features, "
	                          "use the min and max provided in the files (see {min,max}{Node,Edge}Features)."
);

util::ProgramOption optionMinNodeFeatures(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "minNodeFeatures",
	util::_description_text = "A file containing the minimal values of the node features.",
	util::_default_value    = "node_features_min.txt"
);

util::ProgramOption optionMaxNodeFeatures(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "maxNodeFeatures",
	util::_description_text = "A file containing the minimal values of the node features.",
	util::_default_value    = "node_features_max.txt"
);

util::ProgramOption optionMinEdgeFeatures(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "minEdgeFeatures",
	util::_description_text = "A file containing the minimal values of the edge features.",
	util::_default_value    = "edge_features_min.txt"
);

util::ProgramOption optionMaxEdgeFeatures(
	util::_module           = "candidate_mc.features",
	util::_long_name        = "maxEdgeFeatures",
	util::_description_text = "A file containing the minimal values of the edge features.",
	util::_default_value    = "edge_features_max.txt"
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
FeatureExtractor::extract(
		NodeFeatures& nodeFeatures,
		EdgeFeatures& edgeFeatures) {

	extractNodeFeatures(nodeFeatures);
	extractEdgeFeatures(nodeFeatures, edgeFeatures);
}

void
FeatureExtractor::extractNodeFeatures(NodeFeatures& nodeFeatures) {

	int numNodes = 0;
	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
		numNodes++;

	LOG_USER(featureextractorlog) << "extracting features for " << numNodes << " nodes" << std::endl;

	//////////////////////////
	// TOPOLOGICAL FEATURES //
	//////////////////////////

	LOG_DEBUG(featureextractorlog) << "extracting topological node features..." << std::endl;

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

	LOG_DEBUG(featureextractorlog) << "extracting region node features..." << std::endl;

	int i = 0;
	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

		if (i%10 == 0)
			LOG_USER(featureextractorlog) << logger::delline << i << "/" << numNodes << std::flush;
		i++;

		UTIL_TIME_SCOPE("extract node region features");

		LOG_ALL(featureextractorlog)
				<< "extracting features for node "
				<< _crag.id(n) << std::endl;

		// the bounding box of the volume
		const util::box<float, 3>&   nodeBoundingBox    = _crag.getBoundingBox(n);
		util::point<unsigned int, 3> nodeSize           = (nodeBoundingBox.max() - nodeBoundingBox.min())/_crag.getVolume(n).getResolution();
		util::point<float, 3>        nodeOffset         = nodeBoundingBox.min() - _raw.getBoundingBox().min();
		util::point<unsigned int, 3> nodeDiscreteOffset = nodeOffset/_crag.getVolume(n).getResolution();

		LOG_ALL(featureextractorlog)
				<< "node volume bounding box is "
				<< nodeBoundingBox << std::endl;
		LOG_ALL(featureextractorlog)
				<< "node volume discrete size is "
				<< nodeSize << std::endl;

		// a view to the raw image for the node bounding box
		typedef vigra::MultiArrayView<3, float>::difference_type Shape;
		vigra::MultiArrayView<3, float> rawNodeImage =
				_raw.data().subarray(
						Shape(
								nodeDiscreteOffset.x(),
								nodeDiscreteOffset.y(),
								nodeDiscreteOffset.z()),
						Shape(
								nodeDiscreteOffset.x() + nodeSize.x(),
								nodeDiscreteOffset.y() + nodeSize.y(),
								nodeDiscreteOffset.z() + nodeSize.z()));

		LOG_ALL(featureextractorlog)
				<< "raw image has size "
				<< rawNodeImage.shape()
				<< std::endl;

		// the "label" image
		const vigra::MultiArray<3, unsigned char>& labelImage = _crag.getVolume(n).data();

		LOG_ALL(featureextractorlog)
				<< "label image has size "
				<< labelImage.shape()
				<< std::endl;

		// an adaptor to access the feature map with the right node
		FeatureNodeAdaptor adaptor(n, nodeFeatures);

		LOG_ALL(featureextractorlog)
				<< "computing region features..."
				<< std::endl;

		// flat candidates, extract 2D features
		if (_crag.getBoundingBox().depth()/_crag.getVolume(n).getResolution().z() == 1) {

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

		// volumetric candidates
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

			LOG_ALL(featureextractorlog)
					<< "computing region features on boundary map..."
					<< std::endl;

			vigra::MultiArrayView<3, float> boundariesNodeImage =
					_boundaries.data().subarray(
						Shape(
								nodeDiscreteOffset.x(),
								nodeDiscreteOffset.y(),
								nodeDiscreteOffset.z()),
						Shape(
								nodeDiscreteOffset.x() + nodeSize.x(),
								nodeDiscreteOffset.y() + nodeSize.y(),
								nodeDiscreteOffset.z() + nodeSize.z()));

			if (nodeSize.z() == 1) {

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

				LOG_ALL(featureextractorlog)
						<< "computing boundary region features on boundary map..."
						<< std::endl;

				unsigned int width  = nodeSize.x();
				unsigned int height = nodeSize.y();
				unsigned int depth  = nodeSize.z();

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

	LOG_USER(featureextractorlog)
			<< std::endl << "extracted " << nodeFeatures.dims()
			<< " features per node" << std::endl;

	///////////////////
	// NORMALIZATION //
	///////////////////

	if (optionNormalize) {

		LOG_USER(featureextractorlog) << "normalizing features" << std::endl;

		if (optionMinMaxFromFiles) {

			LOG_USER(featureextractorlog)
					<< "reading feature minmax from files"
					<< std::endl;

			std::vector<double> min = retrieveVector<double>(optionMinNodeFeatures);
			std::vector<double> max = retrieveVector<double>(optionMaxNodeFeatures);

			LOG_ALL(featureextractorlog)
					<< "normalizing features with"
					<<  std::endl << min << std::endl
					<< "and"
					<<  std::endl << max << std::endl;

			nodeFeatures.normalize(min, max);

		} else {

			nodeFeatures.normalize();
			storeVector(nodeFeatures.getMin(), optionMinNodeFeatures);
			storeVector(nodeFeatures.getMax(), optionMaxNodeFeatures);
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

	LOG_USER(featureextractorlog) << "extracting edge features..." << std::endl;

	///////////////////
	// NORMALIZATION //
	///////////////////

	if (optionNormalize && edgeFeatures.dims() > 0) {

		LOG_USER(featureextractorlog) << "normalizing features" << std::endl;

		if (optionMinMaxFromFiles) {

			LOG_USER(featureextractorlog)
					<< "reading feature minmax from project file"
					<< std::endl;

			std::vector<double> min = retrieveVector<double>(optionMinEdgeFeatures);
			std::vector<double> max = retrieveVector<double>(optionMaxEdgeFeatures);

			LOG_ALL(featureextractorlog)
					<< "normalizing features with"
					<<  std::endl << min << std::endl
					<< "and"
					<<  std::endl << max << std::endl;

			edgeFeatures.normalize(min, max);

		} else {

			edgeFeatures.normalize();
			storeVector(edgeFeatures.getMin(), optionMinEdgeFeatures);
			storeVector(edgeFeatures.getMax(), optionMaxEdgeFeatures);
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

	LOG_USER(featureextractorlog)
			<< "after postprocessing, we have "
			<< edgeFeatures.dims()
			<< " features per edge" << std::endl;
}
