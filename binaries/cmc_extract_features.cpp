/**
 * Reads a treemc project file, computes features for each region and edge, and 
 * stores them in the project file.
 */

#include <iostream>
#include <boost/filesystem.hpp>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <util/timing.h>
#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <features/FeatureExtractor.h>
#include <features/SkeletonExtractor.h>
#include <features/VolumeRays.h>
#include <features/AccumulatedFeatureProvider.h>
#include <features/AffinityFeatureProvider.h>
#include <features/BiasFeatureProvider.h>
#include <features/CompositeFeatureProvider.h>
#include <features/ContactFeatureProvider.h>
#include <features/DerivedFeatureProvider.h>
#include <features/PairwiseFeatureProvider.h>
#include <features/ShapeFeatureProvider.h>
#include <features/StatisticsFeatureProvider.h>
#include <features/SquareFeatureProvider.h>
#include <features/TopologicalFeatureProvider.h>
#include <features/VolumeRayFeatureProvider.h>
#include <features/ContactFeatureProvider.h>
#include <features/AssignmentFeatureProvider.h>
#include <learning/RandLoss.h>
#include <learning/BestEffort.h>

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The treemc project file.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionAppendBestEffortFeature(
		util::_module           = "features",
		util::_long_name        = "appendBestEffortFeature",
		util::_description_text = "Compute the best-effort from ground-truth and append a binary feature "
		                          "to each node and edge indicating if this node or edge is part of the "
		                          "best-effort solution. Used for testing the learning method.");

util::ProgramOption optionNoFeatures(
		util::_module           = "features",
		util::_long_name        = "noFeatures",
		util::_description_text = "Perform a dry run and don't extract any features (except for "
		                          "best-effort features, if set). Used for testing the learning "
		                          "method.");

///////////////////
// NODE FEATURES //
///////////////////

util::ProgramOption optionNodeShapeFeatures(
		util::_module           = "features.nodes",
		util::_long_name        = "shapeFeatures",
		util::_description_text = "Compute shape features for each candidate."
);

util::ProgramOption optionNodeTopologicalFeatures(
		util::_module           = "features.nodes",
		util::_long_name        = "topologicalFeatures",
		util::_description_text = "Compute topological features for each candidate (like level in the subset graph). "
);

util::ProgramOption optionNodeStatisticsFeatures(
		util::_module           = "features.nodes",
		util::_long_name        = "statisticsFeatures",
		util::_description_text = "Compute statistics features for each candidate (like mean and stddev of intensity, "
		                          "and many more). By default, this computes the statistics over all voxels of the "
		                          "candidate on the raw image."
);

util::ProgramOption optionAssignmentFeatures(
		util::_module           = "features.nodes",
		util::_long_name        = "assignmentFeatures",
		util::_description_text = "Compute assignment node features."
);

///////////////////
// EDGE FEATURES //
///////////////////

util::ProgramOption optionEdgeContactFeatures(
		util::_module           = "features.edges",
		util::_long_name        = "contactFeatures",
		util::_description_text = "Compute contact features as in Gala."
);

util::ProgramOption optionEdgeAccumulatedFeatures(
		util::_module           = "features.edges",
		util::_long_name        = "accumulatedFeatures",
		util::_description_text = "Compute accumulated statistics for each edge (so far on raw data and probability map) "
		                          "(mean, 1-moment, 2-moment)."
);

util::ProgramOption optionEdgeAffinityFeatures(
		util::_module           = "features.edges",
		util::_long_name        = "affinityFeatures",
		util::_description_text = "Compute accumulated statistics for each edge on affinities of affiliated edges "
		                          "(min, 25%, median, 75%, max, mean, 1-moment, 2-moment)."
);

util::ProgramOption optionEdgeVolumeRayFeatures(
		util::_module           = "features.edges",
		util::_long_name        = "volumeRayFeatures",
		util::_description_text = "Compute features based on rays on the surface of the volumes."
);

util::ProgramOption optionEdgeTopologicalFeatures(
		util::_module           = "features.edges",
		util::_long_name        = "topologicalFeatures",
		util::_description_text = "Compute topological features for edges."
);

util::ProgramOption optionEdgeShapeFeatures(
		util::_module           = "features.edges",
		util::_long_name        = "shapeFeatures",
		util::_description_text = "Compute shape features for edges."
);

util::ProgramOption optionEdgeDerivedFeatures(
		util::_module           = "features.edges",
		util::_long_name        = "derivedFeatures",
		util::_description_text = "Compute features for each adjacency edges that are derived from the features of incident candidates "
		                          "(difference, sum, min, max)."
);

/////////////////////////
// STATISTICS FEATURES //
/////////////////////////

util::ProgramOption optionCoordinatesStatistics(
		util::_module           = "features.nodes.statistics",
		util::_long_name        = "coordinatesStatistics",
		util::_description_text = "Include statistics features over voxel coordinates."
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

///////////////////////////////////////////////
// FEATURE NORMALIZATION AND POST-PROCESSING //
///////////////////////////////////////////////

util::ProgramOption optionNormalize(
		util::_module           = "features",
		util::_long_name        = "normalize",
		util::_description_text = "Normalize each original feature to be in the interval [0,1]."
);

util::ProgramOption optionAddFeatureSquares(
		util::_module           = "features",
		util::_long_name        = "addSquares",
		util::_description_text = "For each feature f_i add the square f_i*f_i to the feature vector as well (implied by addPairwiseFeatureProducts)."
);

util::ProgramOption optionAddPairwiseFeatureProducts(
		util::_module           = "features",
		util::_long_name        = "addPairwiseProducts",
		util::_description_text = "For each pair of features f_i and f_j, add the product f_i*f_j to the feature vector as well."
);

util::ProgramOption optionNoFeatureProductsForEdges(
		util::_module           = "features",
		util::_long_name        = "noFeatureProductsForEdges",
		util::_description_text = "Don't add feature products for edges (which can result in too many features)."
);

//////////////////////////
// MORE GENERAL OPTIONS //
//////////////////////////

util::ProgramOption optionMinMaxFromProject(
		util::_module           = "features",
		util::_long_name        = "minMaxFromProject",
		util::_description_text = "Instead of computing the min and max values of the features for normalization, "
		                          "use min and max stored in the project file.");

util::ProgramOption optionSkeletons(
		util::_module           = "features.nodes",
		util::_long_name        = "skeletons",
		util::_description_text = "Extract skeletons for the candidates.");

util::ProgramOption optionVolumeRays(
		util::_module           = "features.nodes",
		util::_long_name        = "volumeRays",
		util::_description_text = "Extract rays locally describing the volume for the candidates.");

util::ProgramOption optionVolumeRaysSampleRadius(
		util::_module           = "features.nodes.rays",
		util::_long_name        = "volumeRaysSampleRadius",
		util::_description_text = "The size of the sphere to use to estimate the surface normal of boundary points.",
		util::_default_value    = 50);

util::ProgramOption optionVolumeRaysSampleDensity(
		util::_module           = "features.nodes.rays",
		util::_long_name        = "volumeRaysSampleDensity",
		util::_description_text = "Distance between sample points in the normal estimation sphere.",
		util::_default_value    = 2);

int main(int argc, char** argv) {

	UTIL_TIME_SCOPE("main");

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		Hdf5CragStore cragStore(optionProjectFile.as<std::string>());

		LOG_USER(logger::out) << "reading CRAG and candidate volumes" << std::endl;

		Crag        crag;
		CragVolumes volumes(crag);
		cragStore.retrieveCrag(crag);
		cragStore.retrieveVolumes(volumes);

		LOG_USER(logger::out) << "reading raw and intensity volumes" << std::endl;

		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());
		ExplicitVolume<float> raw;
		ExplicitVolume<float> boundaries;
		ExplicitVolume<float> xAffinities;
		ExplicitVolume<float> yAffinities;
		ExplicitVolume<float> zAffinities;
		bool hasAffinities = true;

		volumeStore.retrieveIntensities(raw);
		volumeStore.retrieveBoundaries(boundaries);

		try {
			volumeStore.retrieveAffinities(xAffinities, yAffinities, zAffinities);
		} catch (std::exception e) {
			hasAffinities = false;
		}

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		VolumeRays rays(crag);

		if (optionVolumeRays) {

			LOG_USER(logger::out) << "extracting volume rays" << std::endl;

			{
				UTIL_TIME_SCOPE("extracting volume rays");
				rays.extractFromVolumes(volumes, optionVolumeRaysSampleRadius, optionVolumeRaysSampleDensity);
			}

			{
				UTIL_TIME_SCOPE("storing volume rays");
				cragStore.saveVolumeRays(rays);
			}
		}

		if (!optionNoFeatures) {

			LOG_USER(logger::out) << "extracting features" << std::endl;

			// NOTE: Is it needed a feature provider for edges?
			CompositeFeatureProvider featureProvider;

			if (optionNodeShapeFeatures /* || optionEdgeShapeFeatures*/){

				LOG_USER(logger::out) << "\tshape features" << std::endl;

				ShapeFeatureProvider::Parameters p;
				p.numAnglePoints = optionFeaturePointinessAnglePoints;
				p.contourVecAsArcSegmentRatio = optionFeaturePointinessVectorLength;
				p.numAngleHistBins = optionFeaturePointinessHistogramBins;

				featureProvider.emplace_back<ShapeFeatureProvider>(crag, volumes, p);
			}

			if (optionNodeStatisticsFeatures) {

				LOG_USER(logger::out) << "\tnode statistics features" << std::endl;

				StatisticsFeatureProvider::Parameters p;
				p.wholeVolume = true;
				p.boundaryVoxels = false;
				p.computeCoordinateStatistics = optionCoordinatesStatistics;
				featureProvider.emplace_back<StatisticsFeatureProvider>(boundaries, crag, volumes, "membranes ", p);
			}

			if (optionNodeTopologicalFeatures /* || optionEdgeTopologicalFeatures */) {

				LOG_USER(logger::out) << "\tnode topological features" << std::endl;

				featureProvider.emplace_back<TopologicalFeatureProvider>(crag);
			}

			if (optionEdgeContactFeatures) {

				LOG_USER(logger::out) << "\tedge contact features" << std::endl;

				featureProvider.emplace_back<ContactFeatureProvider>(crag, volumes, boundaries);
			}

			if (optionEdgeAccumulatedFeatures) {

				LOG_USER(logger::out) << "\tedge accumulated features" << std::endl;

				featureProvider.emplace_back<AccumulatedFeatureProvider>(crag, boundaries, "membranes");
				featureProvider.emplace_back<AccumulatedFeatureProvider>(crag, raw, "raw");
			}

			if (optionEdgeAffinityFeatures) {

				if (!hasAffinities) {

					UTIL_THROW_EXCEPTION(
							UsageError,
							"asked for affinity features, but no affinities provided");
				}

				LOG_USER(logger::out) << "\tedge affinity features" << std::endl;

				featureProvider.emplace_back<AffinityFeatureProvider>(crag, xAffinities, yAffinities, zAffinities);
			}

			if (optionEdgeDerivedFeatures) {

				LOG_USER(logger::out) << "\tedge derived features" << std::endl;

				featureProvider.emplace_back<DerivedFeatureProvider>(crag, nodeFeatures);
			}

			if (optionEdgeVolumeRayFeatures) {

				LOG_USER(logger::out) << "\tvolume ray features" << std::endl;

				featureProvider.emplace_back<VolumeRayFeatureProvider>(crag, volumes, rays);
			}

			if (optionAssignmentFeatures) {

				LOG_USER(logger::out) << "\tassignment features" << std::endl;

				if (hasAffinities) {

					LOG_USER(logger::out) << "\t\tusing affinity in z direction" << std::endl;
					featureProvider.emplace_back<AssignmentFeatureProvider>(crag, volumes, zAffinities, nodeFeatures);

				} else {

					LOG_USER(logger::out) << "\t\tusing boundaries" << std::endl;
					featureProvider.emplace_back<AssignmentFeatureProvider>(crag, volumes, boundaries, nodeFeatures);
				}
			}

			FeatureExtractor featureExtractor(crag, volumes);
			featureExtractor.extract(featureProvider, nodeFeatures, edgeFeatures);

			LOG_USER(logger::out) << "normalizing features" << std::endl;

			FeatureWeights min, max;

			if (optionMinMaxFromProject) {

				cragStore.retrieveFeaturesMin(min);
				cragStore.retrieveFeaturesMax(max);
			}

			if (optionNormalize)
				featureExtractor.normalize(nodeFeatures, edgeFeatures, min, max);

			LOG_USER(logger::out) << "post-processing features" << std::endl;

			CompositeFeatureProvider postProcessingFeature;
			if (optionAddFeatureSquares)
				postProcessingFeature.emplace_back<SquareFeatureProvider>(crag, !optionNoFeatureProductsForEdges);

			if (optionAddPairwiseFeatureProducts)
				postProcessingFeature.emplace_back<PairwiseFeatureProvider>(crag, !optionNoFeatureProductsForEdges);

			// add bias
			postProcessingFeature.emplace_back<BiasFeatureProvider>(crag, nodeFeatures, edgeFeatures);

			featureExtractor.extract(postProcessingFeature, nodeFeatures, edgeFeatures);

			if (!optionMinMaxFromProject) {

				cragStore.saveFeaturesMin(min);
				cragStore.saveFeaturesMax(max);
			}
		}

		if (optionAppendBestEffortFeature) {

			ExplicitVolume<int> groundTruth;
			volumeStore.retrieveGroundTruth(groundTruth);

			RandLoss randLoss(crag, volumes, groundTruth);
			BestEffort bestEffort(crag, volumes, randLoss);

			for (Crag::CragNode n : crag.nodes())
				if (bestEffort.selected(n))
					nodeFeatures.append(n, 1);
				else
					nodeFeatures.append(n, 0);

			for (Crag::CragEdge e : crag.edges())
				if (bestEffort.selected(e))
					edgeFeatures.append(e, 1);
				else
					edgeFeatures.append(e, 0);
		}

		{
			LOG_USER(logger::out) << "saving features" << std::endl;

			UTIL_TIME_SCOPE("storing features");
			cragStore.saveNodeFeatures(crag, nodeFeatures);
			cragStore.saveEdgeFeatures(crag, edgeFeatures);
		}

		if (optionSkeletons) {

			LOG_USER(logger::out) << "extracting skeletons" << std::endl;

			Skeletons skeletons(crag);

			SkeletonExtractor skeletonExtractor(crag, volumes);
			skeletonExtractor.extract(skeletons);

			{
				UTIL_TIME_SCOPE("storing skeletons");
				cragStore.saveSkeletons(crag, skeletons);
			}
		}

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

