#include <fstream>
#include <sstream>
#include <region_features/RegionFeatures.h>
#include <io/vectors.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/helpers.hpp>
#include <util/timing.h>
#include <util/assert.h>
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
	util::_description_text = "Compute accumulated statistics for each edge (so far on raw data and probability map) "
	                          "(mean, 1-moment, 2-moment). Enabled by default.",
	util::_default_value    = true
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
		EdgeFeatures& edgeFeatures,
		FeatureWeights& min,
		FeatureWeights& max) {

	if (!min.empty() && !max.empty())
		_useProvidedMinMax = true;
	else
		_useProvidedMinMax = false;

	extractNodeFeatures(nodeFeatures, min, max);
	extractEdgeFeatures(nodeFeatures, edgeFeatures, min, max);
}

void
FeatureExtractor::extractNodeFeatures(
		NodeFeatures& nodeFeatures,
		FeatureWeights& min,
		FeatureWeights& max) {

	int numNodes = _crag.nodes().size();

	LOG_USER(featureextractorlog) << "extracting features for " << numNodes << " nodes" << std::endl;

	if (optionNodeShapeFeatures)
		extractNodeShapeFeatures(nodeFeatures);

	if (optionNodeTopologicalFeatures)
		extractTopologicalNodeFeatures(nodeFeatures);

	if (optionNodeStatisticsFeatures)
		extractNodeStatisticsFeatures(nodeFeatures);

	LOG_USER(featureextractorlog)
			<< "extracted " << nodeFeatures.dims(Crag::VolumeNode)
			<< " features per volume node" << std::endl;
	LOG_USER(featureextractorlog)
			<< "extracted " << nodeFeatures.dims(Crag::SliceNode)
			<< " features per slice node" << std::endl;
	LOG_USER(featureextractorlog)
			<< "extracted " << nodeFeatures.dims(Crag::AssignmentNode)
			<< " features per assignment node" << std::endl;

	_numOriginalVolumeNodeFeatures = nodeFeatures.dims(Crag::VolumeNode);

	///////////////////
	// NORMALIZATION //
	///////////////////

	if (optionNormalize) {

		if (_useProvidedMinMax) {

			LOG_USER(featureextractorlog) << "normalizing node features with provided min and max" << std::endl;
			LOG_ALL(featureextractorlog)
					<< "min:" << min << std::endl
					<< "max:" << max << std::endl;

			nodeFeatures.normalize(min, max);

		} else {

			LOG_USER(featureextractorlog) << "normalizing node features" << std::endl;

			nodeFeatures.normalize();
			nodeFeatures.getMin(min);
			nodeFeatures.getMax(max);
		}
	}

	//////////////////////
	//// POSTPROCESSING //
	//////////////////////

	if (optionAddFeatureSquares || optionAddPairwiseFeatureProducts) {

		LOG_USER(featureextractorlog) << "adding feature products" << std::endl;

		for (Crag::CragNode n : _crag.nodes()) {

			const std::vector<double>& features = nodeFeatures[n];

			if (optionAddPairwiseFeatureProducts) {

				// compute all products of all features and add them as well
				unsigned int numOriginalFeatures = features.size();
				for (unsigned int i = 0; i < numOriginalFeatures; i++)
					for (unsigned int j = i; j < numOriginalFeatures; j++)
						nodeFeatures.append(n, features[i]*features[j]);
			} else {

				// compute all squares of all features and add them as well
				unsigned int numOriginalFeatures = features.size();
				for (unsigned int i = 0; i < numOriginalFeatures; i++)
					nodeFeatures.append(n, features[i]*features[i]);
			}
		}
	}

	// append a 1 for bias
	for (Crag::CragNode n : _crag.nodes())
		if (_crag.type(n) != Crag::NoAssignmentNode)
			nodeFeatures.append(n, 1);

	LOG_USER(featureextractorlog)
			<< "after postprocessing, we have "
			<< nodeFeatures.dims(Crag::VolumeNode)
			<< " features per volume node" << std::endl;
	LOG_USER(featureextractorlog)
			<< "after postprocessing, we have "
			<< nodeFeatures.dims(Crag::SliceNode)
			<< " features per slice node" << std::endl;
	LOG_USER(featureextractorlog)
			<< "after postprocessing, we have "
			<< nodeFeatures.dims(Crag::AssignmentNode)
			<< " features per assignment node" << std::endl;

	LOG_USER(featureextractorlog) << "done" << std::endl;
}

void
FeatureExtractor::extractEdgeFeatures(
		const NodeFeatures& nodeFeatures,
		EdgeFeatures&       edgeFeatures,
		FeatureWeights& min,
		FeatureWeights& max) {

	LOG_USER(featureextractorlog) << "extracting edge features..." << std::endl;

	if(optionEdgeAccumulatedeFeatures)
		extractAccumulatedEdgeFeatures(edgeFeatures);

	if (optionEdgeDerivedFeatures)
		extractDerivedEdgeFeatures(nodeFeatures, edgeFeatures);

	LOG_USER(featureextractorlog)
			<< "extracted " << edgeFeatures.dims(Crag::AdjacencyEdge)
			<< " features per adjacency edge" << std::endl;
	LOG_USER(featureextractorlog)
			<< "extracted " << edgeFeatures.dims(Crag::NoAssignmentEdge)
			<< " features per no-assignment edge" << std::endl;

	///////////////////
	// NORMALIZATION //
	///////////////////

	if (optionNormalize && edgeFeatures.dims(Crag::AdjacencyEdge) > 0) {

		if (_useProvidedMinMax) {

			LOG_USER(featureextractorlog) << "normalizing edge features with provided min and max" << std::endl;
			LOG_ALL(featureextractorlog)
					<< "min:" << min << std::endl
					<< "max:" << max << std::endl;

			edgeFeatures.normalize(min, max);

		} else {

			LOG_USER(featureextractorlog) << "normalizing edge features" << std::endl;

			edgeFeatures.normalize();
			edgeFeatures.getMin(min);
			edgeFeatures.getMax(max);
		}
	}

	//////////////////////
	//// POSTPROCESSING //
	//////////////////////

	if (optionAddFeatureSquares || optionAddPairwiseFeatureProducts) {

		LOG_USER(featureextractorlog) << "adding feature products" << std::endl;

		for (Crag::CragEdge e : _crag.edges()) {

			const std::vector<double>& features = edgeFeatures[e];

			if (optionAddPairwiseFeatureProducts) {

				// compute all products of all features and add them as well
				unsigned int numOriginalFeatures = features.size();
				for (unsigned int i = 0; i < numOriginalFeatures; i++)
					for (unsigned int j = i; j < numOriginalFeatures; j++)
						edgeFeatures.append(e, features[i]*features[j]);
			} else {

				// compute all squares of all features and add them as well
				unsigned int numOriginalFeatures = features.size();
				for (unsigned int i = 0; i < numOriginalFeatures; i++)
					edgeFeatures.append(e, features[i]*features[i]);
			}
		}
	}

	// append a 1 for bias
	for (Crag::CragEdge e : _crag.edges())
		if (_crag.type(e) != Crag::AssignmentEdge)
			edgeFeatures.append(e, 1);

	LOG_USER(featureextractorlog)
			<< "after postprocessing, we have "
			<< edgeFeatures.dims(Crag::AdjacencyEdge)
			<< " features per edge" << std::endl;
}

void
FeatureExtractor::extractTopologicalNodeFeatures(NodeFeatures& nodeFeatures) {

	LOG_DEBUG(featureextractorlog) << "extracting topological node features..." << std::endl;

	for (Crag::CragNode n : _crag.nodes())
		if (_crag.isRootNode(n))
			recExtractTopologicalFeatures(nodeFeatures, n);

	_topoFeatureCache.clear();
}

std::pair<int, int>
FeatureExtractor::recExtractTopologicalFeatures(NodeFeatures& nodeFeatures, Crag::CragNode n) {

	if (!_topoFeatureCache.count(n)) {

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

		if (_crag.type(n) != Crag::NoAssignmentNode) {

			nodeFeatures.append(n, level);
			nodeFeatures.append(n, numDescendants);

			_topoFeatureCache[n] = std::make_pair(level, numDescendants);
		}
	}

	return _topoFeatureCache[n];
}

void
FeatureExtractor::extractNodeShapeFeatures(NodeFeatures& nodeFeatures) {

	LOG_DEBUG(featureextractorlog) << "extracting node shape features..." << std::endl;

	int    numAnglePoints              = optionFeaturePointinessAnglePoints;
	double contourVecAsArcSegmentRatio = optionFeaturePointinessVectorLength;
	int    numAngleHistBins            = optionFeaturePointinessHistogramBins;

	int i = 0;
	for (Crag::CragNode n : _crag.nodes()) {

		if (_crag.type(n) == Crag::NoAssignmentNode)
			continue;

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
		if (_crag.type(n) == Crag::SliceNode) {

			RegionFeatures<2, float, unsigned char>::Parameters p;
			p.computeStatistics    = false;
			p.computeShapeFeatures = true;
			p.shapeFeaturesParameters.numAnglePoints              = numAnglePoints;
			p.shapeFeaturesParameters.contourVecAsArcSegmentRatio = contourVecAsArcSegmentRatio;
			p.shapeFeaturesParameters.numAngleHistBins            = numAngleHistBins;
			RegionFeatures<2, float, unsigned char> regionFeatures(labelImage.bind<2>(0), p);

			LOG_ALL(featureextractorlog) << regionFeatures.getFeatureNames() << std::endl;

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

			LOG_ALL(featureextractorlog) << regionFeatures.getFeatureNames() << std::endl;

			regionFeatures.fill(adaptor);
		}
	}
}

void
FeatureExtractor::extractNodeStatisticsFeatures(NodeFeatures& nodeFeatures) {

	LOG_DEBUG(featureextractorlog) << "extracting node statistics features..." << std::endl;

	int i = 0;
	for (Crag::CragNode n : _crag.nodes()) {

		if (_crag.type(n) == Crag::NoAssignmentNode)
			continue;

		if (i%10 == 0)
			LOG_USER(featureextractorlog) << logger::delline << i << "/" << _crag.numNodes() << std::flush;
		i++;

		LOG_ALL(featureextractorlog)
				<< "extracting statistics features for node "
				<< _crag.id(n) << std::endl;

		UTIL_TIME_SCOPE("extract node statistics features");

		// the bounding box of the volume
		const util::box<float, 3>&   nodeBoundingBox    = _volumes[n]->getBoundingBox();
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
		if (_crag.type(n) == Crag::SliceNode) {

			LOG_ALL(featureextractorlog) << "extracting slice features" << std::endl;

			RegionFeatures<2, float, unsigned char>::Parameters p;
			p.computeStatistics    = true;
			p.computeShapeFeatures = false;
			if (optionNoCoordinatesStatistics)
				p.statisticsParameters.computeCoordinateStatistics = false;
			RegionFeatures<2, float, unsigned char> regionFeatures(rawNodeImage.bind<2>(0), labelImage.bind<2>(0), p);

			LOG_ALL(featureextractorlog) << regionFeatures.getFeatureNames() << std::endl;

			regionFeatures.fill(adaptor);

		// volumetric candidates
		} else {

			LOG_ALL(featureextractorlog) << "extracting volumetric features" << std::endl;

			RegionFeatures<3, float, unsigned char>::Parameters p;
			p.computeStatistics    = true;
			p.computeShapeFeatures = false;
			if (optionNoCoordinatesStatistics)
				p.statisticsParameters.computeCoordinateStatistics = false;
			RegionFeatures<3, float, unsigned char> regionFeatures(rawNodeImage, labelImage, p);

			LOG_ALL(featureextractorlog) << regionFeatures.getFeatureNames() << std::endl;

			regionFeatures.fill(adaptor);

			LOG_ALL(featureextractorlog) << "done" << std::endl;
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

			if (_crag.type(n) == Crag::SliceNode) {

				RegionFeatures<2, float, unsigned char>::Parameters p;
				p.computeStatistics    = true;
				p.computeShapeFeatures = false;
				if (optionNoCoordinatesStatistics)
					p.statisticsParameters.computeCoordinateStatistics = false;
				RegionFeatures<2, float, unsigned char> boundariesRegionFeatures(boundariesNodeImage.bind<2>(0), labelImage.bind<2>(0), p);
				boundariesRegionFeatures.fill(adaptor);

				LOG_ALL(featureextractorlog) << boundariesRegionFeatures.getFeatureNames() << std::endl;

			} else {

				RegionFeatures<3, float, unsigned char>::Parameters p;
				p.computeStatistics    = true;
				p.computeShapeFeatures = false;
				if (optionNoCoordinatesStatistics)
					p.statisticsParameters.computeCoordinateStatistics = false;
				RegionFeatures<3, float, unsigned char> boundariesRegionFeatures(boundariesNodeImage, labelImage, p);
				boundariesRegionFeatures.fill(adaptor);

				LOG_ALL(featureextractorlog) << boundariesRegionFeatures.getFeatureNames() << std::endl;
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
				if (_crag.type(n) == Crag::SliceNode)
					vigra::discErosion(labelImage.bind<2>(0), erosionImage.bind<2>(0), 1);
				else
					vigra::multiBinaryErosion(labelImage, erosionImage, 1);
				vigra::MultiArray<3, unsigned char> boundaryImage(labelImage.shape());
				boundaryImage = labelImage;
				boundaryImage -= erosionImage;

				// ...and fix the border of the label image
				if (_crag.type(n) == Crag::SliceNode) {

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

				if (_crag.type(n) == Crag::SliceNode) {

					RegionFeatures<2, float, unsigned char>::Parameters p;
					p.computeStatistics    = true;
					p.computeShapeFeatures = false;
					if (optionNoCoordinatesStatistics)
						p.statisticsParameters.computeCoordinateStatistics = false;
					RegionFeatures<2, float, unsigned char> boundaryFeatures(boundariesNodeImage.bind<2>(0), boundaryImage.bind<2>(0), p);
					boundaryFeatures.fill(adaptor);

					LOG_ALL(featureextractorlog) << boundaryFeatures.getFeatureNames() << std::endl;

				} else {

					RegionFeatures<3, float, unsigned char>::Parameters p;
					p.computeStatistics    = true;
					p.computeShapeFeatures = false;
					if (optionNoCoordinatesStatistics)
						p.statisticsParameters.computeCoordinateStatistics = false;
					RegionFeatures<3, float, unsigned char> boundaryFeatures(boundariesNodeImage, boundaryImage, p);
					boundaryFeatures.fill(adaptor);

					LOG_ALL(featureextractorlog) << boundaryFeatures.getFeatureNames() << std::endl;
				}
			}
		}
	}

	LOG_USER(featureextractorlog) << std::endl;
}

void
FeatureExtractor::extractDerivedEdgeFeatures(const NodeFeatures& nodeFeatures, EdgeFeatures& edgeFeatures) {

	////////////////////////
	// Edge Features      //
	// from Node Features //
	////////////////////////
	for (Crag::CragEdge e : _crag.edges()) {

		if (_crag.type(e) != Crag::AdjacencyEdge)
			continue;

		Crag::CragNode u = e.u();
		Crag::CragNode v = e.v();
		std::cout << _crag.id(u) << " " << _crag.id(v) << std::endl;
		// feature vectors from node u/v
		const auto & featsU = nodeFeatures[u];
		const auto & featsV = nodeFeatures[v];

		UTIL_ASSERT(_crag.type(u) == Crag::VolumeNode || _crag.type(u) == Crag::SliceNode);
		UTIL_ASSERT(_crag.type(v) == Crag::VolumeNode || _crag.type(v) == Crag::SliceNode);
		UTIL_ASSERT_REL(featsU.size(), ==, featsV.size());
		UTIL_ASSERT_REL(featsU.size(), >=, _numOriginalVolumeNodeFeatures);
		UTIL_ASSERT_REL(featsV.size(), >=, _numOriginalVolumeNodeFeatures);

		// loop over all features
		for(size_t nfi=0; nfi < _numOriginalVolumeNodeFeatures; ++nfi){
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
	for (Crag::CragEdge e : _crag.edges()) {

		if (_crag.type(e) != Crag::AdjacencyEdge)
			continue;

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
	for (Crag::CragNode n : _crag.nodes()) {

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

	for (Crag::CragEdge e : _crag.edges())
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
