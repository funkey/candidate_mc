#include <fstream>
#include <sstream>
#include <region_features/RegionFeatures.h>
#include <io/vectors.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/helpers.hpp>
#include <util/timing.h>
#include "FeatureExtractor.h"
#include <vigra/multi_impex.hxx>
#include <vigra/flatmorphology.hxx>
#include <vigra/multi_morphology.hxx>


#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>



logger::LogChannel featureextractorlog("featureextractorlog", "[FeatureExtractor] ");

/////////////////////
// GENERAL OPTIONS //
/////////////////////

// FEATURE GROUPS

util::ProgramOption optionNodeShapeFeatures(
	util::_module           = "features.nodes",
	util::_long_name        = "shapeFeatures",
	util::_description_text = "Compute shape features for each candidate. Enabled by default.",
	util::_default_value    = true
);

util::ProgramOption optionNodeTopologicalFeatures(
	util::_module           = "features.nodes",
	util::_long_name        = "topologicalFeatures",
	util::_description_text = "Compute topological features for each candidate (like level in the subset graph). "
							  "Enabled by default.",
	util::_default_value    = true
);

util::ProgramOption optionNodeStatisticsFeatures(
	util::_module           = "features.nodes",
	util::_long_name        = "statisticsFeatures",
	util::_description_text = "Compute statistics features for each candidate (like mean and stddev of intensity, "
							  "and many more). By default, this computes the statistics over all voxels of the "
							  "candidate on the raw image. Enabled by default.",
	util::_default_value    = true
);

util::ProgramOption optionEdgeDerivedFeatures(
	util::_module           = "features.edges",
	util::_long_name        = "derivedFeatures",
	util::_description_text = "Compute features for each adjacency edges that are derived from the features of incident candidates "
							  "(difference, sum, min, max). Enabled by default.",
	util::_default_value    = true
);

util::ProgramOption optionEdgeAccumulatedeFeatures(
	util::_module           = "features.edges",
	util::_long_name        = "accumulatedFeatures",
	util::_description_text = "Compute accumulated statistics for each each (so far on raw data and probability map) "
							  "(mean, 1-moment, 2-moment). Enabled by default.",
	util::_default_value    = true
);

util::ProgramOption optionEdgeSegmentationFeatures(
	util::_module           = "features.edges",
	util::_long_name        = "segmentationFeatures",
	util::_description_text = "Compute a feature for each adjacency edge that reflects how many times the incident candidates ended "
							  "up in the same segment. For that, a list of segmentations has to be provided. Disabled by default."
);

// FEATURE NORMALIZATION AND POST-PROCESSING

util::ProgramOption optionAddPairwiseFeatureProducts(
	util::_module           = "features",
	util::_long_name        = "addPairwiseProducts",
	util::_description_text = "For each pair of features f_i and f_j, add the product f_i*f_j to the feature vector as well."
);

util::ProgramOption optionAddFeatureSquares(
	util::_module           = "features",
	util::_long_name        = "addSquares",
	util::_description_text = "For each feature f_i add the square f_i*f_i to the feature vector as well (implied by addPairwiseFeatureProducts)."
);

util::ProgramOption optionNormalize(
	util::_module           = "features",
	util::_long_name        = "normalize",
	util::_description_text = "Normalize each original feature to be in the interval [0,1]."
);

util::ProgramOption optionMinMaxFromFiles(
	util::_module           = "features",
	util::_long_name        = "minMaxFromFiles",
	util::_description_text = "For the feature normalization, instead of using the min and max of the extracted features, "
							  "use the min and max provided in the files (see {min,max}{Node,Edge}Features)."
);

util::ProgramOption optionMinNodeFeatures(
	util::_module           = "features",
	util::_long_name        = "minNodeFeatures",
	util::_description_text = "A file containing the minimal values of the node features.",
	util::_default_value    = "node_features_min.txt"
);

util::ProgramOption optionMaxNodeFeatures(
	util::_module           = "features",
	util::_long_name        = "maxNodeFeatures",
	util::_description_text = "A file containing the minimal values of the node features.",
	util::_default_value    = "node_features_max.txt"
);

util::ProgramOption optionMinEdgeFeatures(
	util::_module           = "features",
	util::_long_name        = "minEdgeFeatures",
	util::_description_text = "A file containing the minimal values of the edge features.",
	util::_default_value    = "edge_features_min.txt"
);

util::ProgramOption optionMaxEdgeFeatures(
	util::_module           = "features",
	util::_long_name        = "maxEdgeFeatures",
	util::_description_text = "A file containing the minimal values of the edge features.",
	util::_default_value    = "edge_features_max.txt"
);

/////////////////////////
// STATISTICS FEATURES //
/////////////////////////

util::ProgramOption optionBoundariesFeatures(
	util::_module           = "features.statistics",
	util::_long_name        = "boundariesFeatures",
	util::_description_text = "Compute the statistics features also over all boundary voxels of each candidate."
);

util::ProgramOption optionBoundariesBoundaryFeatures(
	util::_module           = "features.statistics",
	util::_long_name        = "boundariesBoundaryFeatures",
	util::_description_text = "Compute the statistics features also over all boundary voxels of each candidate on "
							  "the boundary probability image."
);

util::ProgramOption optionNoCoordinatesStatistics(
	util::_module           = "features.statistics",
	util::_long_name        = "noCoordinatesStatistics",
	util::_description_text = "Do not include statistics features over voxel coordinates."
);

////////////////////
// SHAPE FEATURES //
////////////////////

util::ProgramOption optionFeaturePointinessAnglePoints(
	util::_module           = "features.shape.pointiness",
	util::_long_name        = "numAnglePoints",
	util::_description_text = "The number of points to sample equidistantly on the contour of nodes. Default is 50.",
	util::_default_value    = 50
);

util::ProgramOption optionFeaturePointinessVectorLength(
	util::_module           = "features.shape.pointiness",
	util::_long_name        = "angleVectorLength",
	util::_description_text = "The amount to walk on the contour from a sample point in either direction, to estimate the angle. Values are between "
							  "0 (at the sample point) and 1 (at the next sample point). Default is 0.1.",
	util::_default_value    = 0.1
);

util::ProgramOption optionFeaturePointinessHistogramBins(
	util::_module           = "features.shape.pointiness",
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

	visualizeEdgeFeatures(edgeFeatures);
}

void
FeatureExtractor::extractNodeFeatures(NodeFeatures& nodeFeatures) {

	int numNodes = 0;
	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
		numNodes++;

	LOG_USER(featureextractorlog) << "extracting features for " << numNodes << " nodes" << std::endl;

	if (optionNodeShapeFeatures)
		extractNodeShapeFeatures(nodeFeatures);

	if (optionNodeTopologicalFeatures)
		extractTopologicalNodeFeatures(nodeFeatures);

	if (optionNodeStatisticsFeatures)
		extractNodeStatisticsFeatures(nodeFeatures);

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

void
FeatureExtractor::extractEdgeFeatures(
		const NodeFeatures& nodeFeatures,
		EdgeFeatures&       edgeFeatures) {

	LOG_USER(featureextractorlog) << "extracting edge features..." << std::endl;

	if(optionEdgeAccumulatedeFeatures)
		extractAccumulatedEdgeFeatures(edgeFeatures);

	if (optionEdgeDerivedFeatures)
		extractDerivedEdgeFeatures(nodeFeatures, edgeFeatures);

	if (optionEdgeSegmentationFeatures)
		extractEdgeSegmentationFeatures(edgeFeatures);

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

void
FeatureExtractor::extractTopologicalNodeFeatures(NodeFeatures& nodeFeatures) {

	LOG_DEBUG(featureextractorlog) << "extracting topological node features..." << std::endl;

	for (Crag::CragNode n : _crag.nodes())
		if (_crag.isRootNode(n))
			recExtractTopologicalFeatures(nodeFeatures, n);
}

std::pair<int, int>
FeatureExtractor::recExtractTopologicalFeatures(NodeFeatures& nodeFeatures, Crag::CragNode n) {

	int numChildren    = 0;
	int numDescendants = 0;
	int level          = 1; // level of leaf nodes

	for (Crag::CragArc e : _crag.inArcs(n)) {

		std::pair<int, int> level_descendants =
				recExtractTopologicalFeatures(
						nodeFeatures,
						e.source());

		level = std::max(level, level_descendants.first + 1);
		numDescendants += level_descendants.second;
		numChildren++;
	}

	numDescendants += numChildren;

	nodeFeatures.append(n, level);
	nodeFeatures.append(n, numDescendants);

	return std::make_pair(level, numDescendants);
}

void
FeatureExtractor::extractNodeShapeFeatures(NodeFeatures& nodeFeatures) {

	LOG_DEBUG(featureextractorlog) << "extracting node shape features..." << std::endl;

	int    numAnglePoints              = optionFeaturePointinessAnglePoints;
	double contourVecAsArcSegmentRatio = optionFeaturePointinessVectorLength;
	int    numAngleHistBins            = optionFeaturePointinessHistogramBins;

	int i = 0;
	for (Crag::CragNode n : _crag.nodes()) {

		if (i%10 == 0)
			LOG_USER(featureextractorlog) << logger::delline << i << "/" << _crag.numNodes() << std::flush;
		i++;

		LOG_ALL(featureextractorlog)
				<< "extracting shape features for node "
				<< _crag.id(n) << std::endl;

		UTIL_TIME_SCOPE("extract node shape features");

		// the "label" image
		const vigra::MultiArray<3, unsigned char>& labelImage = _volumes[n]->data();

		// an adaptor to access the feature map with the right node
		FeatureNodeAdaptor adaptor(n, nodeFeatures);

		// flat candidates, extract 2D features
		if (_volumes.getBoundingBox().depth()/_volumes[n]->getResolution().z() == 1) {

			RegionFeatures<2, float, unsigned char>::Parameters p;
			p.computeStatistics    = false;
			p.computeShapeFeatures = true;
			p.shapeFeaturesParameters.numAnglePoints              = numAnglePoints;
			p.shapeFeaturesParameters.contourVecAsArcSegmentRatio = contourVecAsArcSegmentRatio;
			p.shapeFeaturesParameters.numAngleHistBins            = numAngleHistBins;
			RegionFeatures<2, float, unsigned char> regionFeatures(labelImage.bind<2>(0), p);

			if (_crag.id(n) == 0)
				LOG_DEBUG(featureextractorlog) << regionFeatures.getFeatureNames() << std::endl;

			regionFeatures.fill(adaptor);

		// volumetric candidates
		} else {

			RegionFeatures<3, float, unsigned char>::Parameters p;
			p.computeStatistics    = false;
			p.computeShapeFeatures = true;
			p.shapeFeaturesParameters.numAnglePoints              = numAnglePoints;
			p.shapeFeaturesParameters.contourVecAsArcSegmentRatio = contourVecAsArcSegmentRatio;
			p.shapeFeaturesParameters.numAngleHistBins            = numAngleHistBins;
			RegionFeatures<3, float, unsigned char> regionFeatures(labelImage, p);

			if (_crag.id(n) == 0)
				LOG_DEBUG(featureextractorlog) << regionFeatures.getFeatureNames() << std::endl;

			regionFeatures.fill(adaptor);
		}
	}
}

void
FeatureExtractor::extractNodeStatisticsFeatures(NodeFeatures& nodeFeatures) {

	LOG_DEBUG(featureextractorlog) << "extracting node statistics features..." << std::endl;

	int i = 0;
	for (Crag::CragNode n : _crag.nodes()) {

		if (i%10 == 0)
			LOG_USER(featureextractorlog) << logger::delline << i << "/" << _crag.numNodes() << std::flush;
		i++;

		LOG_ALL(featureextractorlog)
				<< "extracting statistics features for node "
				<< _crag.id(n) << std::endl;

		UTIL_TIME_SCOPE("extract node statistics features");

		// the bounding box of the volume
		const util::box<float, 3>&   nodeBoundingBox    = _volumes.getBoundingBox(n);
		util::point<unsigned int, 3> nodeSize           = (nodeBoundingBox.max() - nodeBoundingBox.min())/_volumes[n]->getResolution();
		util::point<float, 3>        nodeOffset         = nodeBoundingBox.min() - _raw.getBoundingBox().min();
		util::point<unsigned int, 3> nodeDiscreteOffset = nodeOffset/_volumes[n]->getResolution();

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
		const vigra::MultiArray<3, unsigned char>& labelImage = _volumes[n]->data();

		LOG_ALL(featureextractorlog)
				<< "label image has size "
				<< labelImage.shape()
				<< std::endl;

		// an adaptor to access the feature map with the right node
		FeatureNodeAdaptor adaptor(n, nodeFeatures);

		// flat candidates, extract 2D features
		if (_volumes.getBoundingBox().depth()/_volumes[n]->getResolution().z() == 1) {

			RegionFeatures<2, float, unsigned char>::Parameters p;
			p.computeStatistics    = true;
			p.computeShapeFeatures = false;
			if (optionNoCoordinatesStatistics)
				p.statisticsParameters.computeCoordinateStatistics = false;
			RegionFeatures<2, float, unsigned char> regionFeatures(rawNodeImage.bind<2>(0), labelImage.bind<2>(0), p);

			if (_crag.id(n) == 0)
				LOG_DEBUG(featureextractorlog) << regionFeatures.getFeatureNames() << std::endl;

			regionFeatures.fill(adaptor);

		// volumetric candidates
		} else {

			RegionFeatures<3, float, unsigned char>::Parameters p;
			p.computeStatistics    = true;
			p.computeShapeFeatures = false;
			if (optionNoCoordinatesStatistics)
				p.statisticsParameters.computeCoordinateStatistics = false;
			RegionFeatures<3, float, unsigned char> regionFeatures(rawNodeImage, labelImage, p);

			if (_crag.id(n) == 0)
				LOG_DEBUG(featureextractorlog) << regionFeatures.getFeatureNames() << std::endl;

			regionFeatures.fill(adaptor);
		}

		if (optionBoundariesFeatures) {

			LOG_ALL(featureextractorlog)
					<< "computing node statistics features on boundary map..."
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
				p.computeStatistics    = true;
				p.computeShapeFeatures = false;
				if (optionNoCoordinatesStatistics)
					p.statisticsParameters.computeCoordinateStatistics = false;
				RegionFeatures<2, float, unsigned char> boundariesRegionFeatures(boundariesNodeImage.bind<2>(0), labelImage.bind<2>(0), p);
				boundariesRegionFeatures.fill(adaptor);

				if (_crag.id(n) == 0)
					LOG_DEBUG(featureextractorlog) << boundariesRegionFeatures.getFeatureNames() << std::endl;

			} else {

				RegionFeatures<3, float, unsigned char>::Parameters p;
				p.computeStatistics    = true;
				p.computeShapeFeatures = false;
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

				// create the boundary image by erosion and subtraction...
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
					p.computeStatistics    = true;
					p.computeShapeFeatures = false;
					if (optionNoCoordinatesStatistics)
						p.statisticsParameters.computeCoordinateStatistics = false;
					RegionFeatures<2, float, unsigned char> boundaryFeatures(boundariesNodeImage.bind<2>(0), boundaryImage.bind<2>(0), p);
					boundaryFeatures.fill(adaptor);

					if (_crag.id(n) == 0)
						LOG_DEBUG(featureextractorlog) << boundaryFeatures.getFeatureNames() << std::endl;

				} else {

					RegionFeatures<3, float, unsigned char>::Parameters p;
					p.computeStatistics    = true;
					p.computeShapeFeatures = false;
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
}

void
FeatureExtractor::extractEdgeSegmentationFeatures(EdgeFeatures& edgeFeatures) {

	if (_segmentations.size() > 0) {

		UTIL_TIME_SCOPE("extract edge segmentation features");

		for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e) {

			Crag::Node u = _crag.u(e);
			Crag::Node v = _crag.v(e);

			// count how many times u and v were connected in all segmentations
			int connected = 0;
			for (auto& segmentation : _segmentations)
				for (auto& segment : segmentation)
					if (segment.count(u) && segment.count(v))
						connected++;

			edgeFeatures.append(e, connected);
		}
	}
}

void
FeatureExtractor::extractDerivedEdgeFeatures(const NodeFeatures& nodeFeatures, EdgeFeatures& edgeFeatures) {

	////////////////////////
	// Edge Features      //
	// from Node Features //
	////////////////////////
	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e) {
		Crag::Node u = _crag.u(e);
		Crag::Node v = _crag.v(e);
		// feature vectors from node u/v
		const auto & featsU = nodeFeatures[u];
		const auto & featsV = nodeFeatures[v];
		// loop over all features
		for(size_t nfi=0; nfi < featsU.size(); ++nfi){
			// single feature from node u/v
			const auto fu = featsU[nfi];
			const auto fv = featsV[nfi];
			// convert u/v features into 
			// edge features
			edgeFeatures.append(e, std::abs(fu-fv));
			edgeFeatures.append(e, std::min(fu,fv));
			edgeFeatures.append(e, std::max(fu,fv));
			edgeFeatures.append(e, fu+fv);
		}
	}
}

void
FeatureExtractor::extractAccumulatedEdgeFeatures(EdgeFeatures & edgeFeatures){

	const auto & gridGraph = _crag.getGridGraph();

	// iterate over all edges
	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e){

		// accumulate statistics
		
		// Define an accumulator set for calculating the mean and the
		// 2nd moment .
		using namespace boost::accumulators;
		typedef  stats<
			tag::mean, 
			tag::moment<1>,
			tag::moment<2> 
		> Stats;
		accumulator_set<double, Stats> accBoundary;
		accumulator_set<double, Stats> accRaw;

		// push data into the accumulator
		for (vigra::GridGraph<3>::Edge ae : _crag.getAffiliatedEdges(e)) {
			const auto ggU = gridGraph.u(ae);
			const auto ggV = gridGraph.v(ae);

			accBoundary(_boundaries[ggU]);
			accBoundary(_boundaries[ggV]);

			accRaw(_raw[ggU]);
			accRaw(_raw[ggV]);
		}

		// extract the features from the accumulator
		edgeFeatures.append(e, mean(accBoundary));
		edgeFeatures.append(e, moment<1>(accBoundary));
		edgeFeatures.append(e, moment<2>(accBoundary));

		edgeFeatures.append(e, mean(accRaw));
		edgeFeatures.append(e, moment<1>(accRaw));
		edgeFeatures.append(e, moment<2>(accRaw));
	}
}
	
void
FeatureExtractor::visualizeEdgeFeatures(const EdgeFeatures& edgeFeatures) {

	LOG_USER(featureextractorlog) << "visualizing edge features... " << std::flush;

	util::box<float, 3>   cragBB = _volumes.getBoundingBox();
	util::point<float, 3> resolution;
	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

		if (!_crag.isLeafNode(n))
			continue;
		resolution = _volumes[n]->getResolution();
		break;
	}

	// create a vigra multi-array large enough to hold all volumes
	vigra::MultiArray<3, float> edges(
			vigra::Shape3(
				cragBB.width() /resolution.x(),
				cragBB.height()/resolution.y(),
				cragBB.depth() /resolution.z()),
			std::numeric_limits<int>::max());
	edges = 0;

	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e)
		for (vigra::GridGraph<3>::Edge ae : _crag.getAffiliatedEdges(e)) {

			edges[_crag.getGridGraph().u(ae)] = edgeFeatures[e][0];
			edges[_crag.getGridGraph().v(ae)] = edgeFeatures[e][0];
		}

	boost::filesystem::create_directory("affinities");

	for (unsigned int z = 0; z < edges.shape(2); z++) {

		std::stringstream ss;
		ss << std::setw(4) << std::setfill('0') << z;
		vigra::exportImage(
				edges.bind<2>(z),
				vigra::ImageExportInfo((std::string("affinities/") + ss.str() + ".tif").c_str()));
	}

	LOG_USER(featureextractorlog) << "done" << std::endl;
}
