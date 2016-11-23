#include <fstream>
#include <sstream>
#include <io/vectors.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/helpers.hpp>
#include <util/timing.h>
#include <util/assert.h>
#include <util/geometry.hpp>
#include "FeatureExtractor.h"
#include "VolumeRayFeature.h"
#include <vigra/multi_impex.hxx>

logger::LogChannel featureextractorlog("featureextractorlog", "[FeatureExtractor] ");

/////////////////////
// GENERAL OPTIONS //
/////////////////////

// FEATURE GROUPS

util::ProgramOption optionEdgeDerivedFeatures(
	util::_module           = "features.edges",
	util::_long_name        = "derivedFeatures",
	util::_description_text = "Compute features for each adjacency edges that are derived from the features of incident candidates "
							  "(difference, sum, min, max). Enabled by default.",
	util::_default_value    = true
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

util::ProgramOption optionEdgeVolumeRayFeatures(
	util::_module           = "features.edges",
	util::_long_name        = "volumeRayFeatures",
	util::_description_text = "Compute features based on rays on the surface of the volumes. Disabled by default."
);

// FEATURE NORMALIZATION AND POST-PROCESSING

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

util::ProgramOption optionDumpFeatureNames(
	util::_long_name        = "dumpFeatureNames",
	util::_description_text = "Write the feature names to files. The filenames will be the value of this argument plus 'node_?' or 'edge_?' "
	                          "for the respective node and edge types."
);

void
FeatureExtractor::extract(
		FeatureProviderBase& featureProvider,
		NodeFeatures& nodeFeatures,
		EdgeFeatures& edgeFeatures,
		FeatureWeights& min,
		FeatureWeights& max) {

	if (!min.empty() && !max.empty())
		_useProvidedMinMax = true;
	else
		_useProvidedMinMax = false;

	extractNodeFeatures(featureProvider, nodeFeatures, min, max);
	extractEdgeFeatures(featureProvider, nodeFeatures, edgeFeatures, min, max);
}

void
FeatureExtractor::extractNodeFeatures(
		FeatureProviderBase& featureProvider,
		NodeFeatures& nodeFeatures,
		FeatureWeights& min,
		FeatureWeights& max) {

	int numNodes = _crag.nodes().size();

	LOG_USER(featureextractorlog) << "extracting features for " << numNodes << " nodes" << std::endl;

	featureProvider.appendFeatures(_crag, nodeFeatures);

	LOG_USER(featureextractorlog)
			<< "extracted " << nodeFeatures.dims(Crag::VolumeNode)
			<< " features per volume node" << std::endl;
	LOG_USER(featureextractorlog)
			<< "extracted " << nodeFeatures.dims(Crag::SliceNode)
			<< " features per slice node" << std::endl;
	LOG_USER(featureextractorlog)
			<< "extracted " << nodeFeatures.dims(Crag::AssignmentNode)
			<< " features per assignment node" << std::endl;

	LOG_DEBUG(featureextractorlog)
			<< "base volume node features: " << nodeFeatures.getFeatureNames(Crag::VolumeNode)
			<< std::endl;
	LOG_DEBUG(featureextractorlog)
			<< "base slice node features: " << nodeFeatures.getFeatureNames(Crag::SliceNode)
			<< std::endl;
	LOG_DEBUG(featureextractorlog)
			<< "base assignment node features: " << nodeFeatures.getFeatureNames(Crag::AssignmentNode)
			<< std::endl;

	_numOriginalVolumeNodeFeatures     = nodeFeatures.dims(Crag::VolumeNode);
	_numOriginalSliceNodeFeatures      = nodeFeatures.dims(Crag::SliceNode);
	_numOriginalAssignmentNodeFeatures = nodeFeatures.dims(Crag::AssignmentNode);

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


		for (auto type : Crag::NodeTypes) {

			std::vector<std::string> baseNames = nodeFeatures.getFeatureNames(type);

			if (optionAddPairwiseFeatureProducts) {

				for (unsigned int i = 0; i < baseNames.size(); i++)
					for (unsigned int j = i; j < baseNames.size(); j++)
						nodeFeatures.appendFeatureName(type, baseNames[i] + "*" + baseNames[j]);

			} else {

				for (unsigned int i = 0; i < baseNames.size(); i++)
					nodeFeatures.appendFeatureName(type, baseNames[i]+"²");
			}
		}

	}

	// append a 1 for bias
	for (Crag::CragNode n : _crag.nodes())
		if (_crag.type(n) != Crag::NoAssignmentNode)
			nodeFeatures.append(n, 1);

	for (auto type : Crag::NodeTypes)
		if (nodeFeatures.getFeatureNames(type).size() > 0)
			nodeFeatures.appendFeatureName(type, "bias");

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

	if (optionDumpFeatureNames) {

		for (auto type : Crag::NodeTypes) {

			std::string filename = optionDumpFeatureNames.as<std::string>() + "node_" + boost::lexical_cast<std::string>(type);
			std::ofstream file(filename);

			file << "number of features: " << nodeFeatures.dims(type) << "\n";
			file << "number of names: " << nodeFeatures.getFeatureNames(type).size() << "\n";

			for (auto name : nodeFeatures.getFeatureNames(type))
				file << name << "\n";
			file.close();
		}
	}

	LOG_USER(featureextractorlog) << "done" << std::endl;
}

void
FeatureExtractor::extractEdgeFeatures(
		FeatureProviderBase& featureProvider,
		const NodeFeatures& nodeFeatures,
		EdgeFeatures&       edgeFeatures,
		FeatureWeights& min,
		FeatureWeights& max) {

	LOG_USER(featureextractorlog) << "extracting edge features..." << std::endl;

	featureProvider.appendFeatures(_crag, edgeFeatures);

	if (optionEdgeDerivedFeatures)
		extractDerivedEdgeFeatures(nodeFeatures, edgeFeatures);

	if (optionEdgeTopologicalFeatures)
		extractTopologicalEdgeFeatures(edgeFeatures);

	if (optionEdgeShapeFeatures)
		extractShapeEdgeFeatures(nodeFeatures, edgeFeatures);

	if (optionEdgeVolumeRayFeatures)
		extractVolumeRaysEdgeFeatures(edgeFeatures);

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

	if ((optionAddFeatureSquares || optionAddPairwiseFeatureProducts) && !optionNoFeatureProductsForEdges) {

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
		if (_crag.type(e) != Crag::AssignmentEdge && _crag.type(e) != Crag::SeparationEdge)
			edgeFeatures.append(e, 1);

	LOG_USER(featureextractorlog)
			<< "after postprocessing, we have "
			<< edgeFeatures.dims(Crag::AdjacencyEdge)
			<< " features per adjacency edge" << std::endl;
	LOG_USER(featureextractorlog)
			<< "after postprocessing, we have "
			<< edgeFeatures.dims(Crag::NoAssignmentEdge)
			<< " features per no-assignment edge" << std::endl;

    if (optionDumpFeatureNames) {

        for (auto type : Crag::EdgeTypes) {

            std::string filename = optionDumpFeatureNames.as<std::string>() + "edge_" + boost::lexical_cast<std::string>(type);
            std::ofstream file(filename);

			file << "number of features: " << edgeFeatures.dims(type) << "\n";
			file << "number of names: " << edgeFeatures.getFeatureNames(type).size() << "\n";

            for (auto name : edgeFeatures.getFeatureNames(type))
                file << name << "\n";
            file.close();
        }
    }

    LOG_USER(featureextractorlog) << "done" << std::endl;
}

void
FeatureExtractor::extractDerivedEdgeFeatures(const NodeFeatures& nodeFeatures, EdgeFeatures& edgeFeatures) {

	LOG_DEBUG(featureextractorlog) << "extracting derived edge features..." << std::endl;

	if (_crag.edges().size() == 0)
		return;

	int numOriginalNodeFeatures = 0;
	for (Crag::CragEdge e : _crag.edges()) {

		if (_crag.type(e) != Crag::AdjacencyEdge)
			continue;

		switch (_crag.type(e.u())) {

			case Crag::VolumeNode:
				numOriginalNodeFeatures = _numOriginalVolumeNodeFeatures;
				break;
			case Crag::SliceNode:
				numOriginalNodeFeatures = _numOriginalSliceNodeFeatures;
				break;
			default:
				numOriginalNodeFeatures = _numOriginalAssignmentNodeFeatures;
		}
		break;
	}

	for (size_t i = 0; i < numOriginalNodeFeatures; i++) {

		edgeFeatures.appendFeatureName(Crag::AdjacencyEdge, std::string("derived_node_abs_") + boost::lexical_cast<std::string>(i));
		edgeFeatures.appendFeatureName(Crag::AdjacencyEdge, std::string("derived_node_min_") + boost::lexical_cast<std::string>(i));
		edgeFeatures.appendFeatureName(Crag::AdjacencyEdge, std::string("derived_node_max_") + boost::lexical_cast<std::string>(i));
		edgeFeatures.appendFeatureName(Crag::AdjacencyEdge, std::string("derived_node_sum_") + boost::lexical_cast<std::string>(i));
	}

	////////////////////////
	// Edge Features      //
	// from Node Features //
	////////////////////////
	for (Crag::CragEdge e : _crag.edges()) {

		if (_crag.type(e) != Crag::AdjacencyEdge)
			continue;

		LOG_ALL(featureextractorlog) << "extracting derived features for edge " << _crag.id(_crag.u(e)) << ", " << _crag.id(_crag.v(e)) << std::endl;

		UTIL_TIME_SCOPE("extract derived edge features");

		Crag::CragNode u = e.u();
		Crag::CragNode v = e.v();
		// feature vectors from node u/v
		const auto & featsU = nodeFeatures[u];
		const auto & featsV = nodeFeatures[v];

		UTIL_ASSERT(_crag.type(u) == _crag.type(v));
		UTIL_ASSERT_REL(featsU.size(), ==, featsV.size());
		UTIL_ASSERT_REL(featsU.size(), >=, numOriginalNodeFeatures);
		UTIL_ASSERT_REL(featsV.size(), >=, numOriginalNodeFeatures);

		// loop over all features
		for(size_t nfi=0; nfi < numOriginalNodeFeatures; ++nfi){
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
FeatureExtractor::extractTopologicalEdgeFeatures(EdgeFeatures& edgeFeatures) {

	LOG_DEBUG(featureextractorlog) << "extracting topological edge features..." << std::endl;

	for (auto type : Crag::EdgeTypes) {

		edgeFeatures.appendFeatureName(type, "min_level");
		edgeFeatures.appendFeatureName(type, "max_level");
		edgeFeatures.appendFeatureName(type, "siblings");
		edgeFeatures.appendFeatureName(type, "root_edge");
		edgeFeatures.appendFeatureName(type, "leaf_edge");
	}

	for (Crag::CragEdge e : _crag.edges()) {

		Crag::CragNode u = e.u();
		Crag::CragNode v = e.v();

		edgeFeatures.append(e, std::min(_crag.getLevel(u), _crag.getLevel(v)));
		edgeFeatures.append(e, std::max(_crag.getLevel(u), _crag.getLevel(v)));

		Crag::CragNode pu = (_crag.outArcs(u).size() > 0 ? (*_crag.outArcs(u).begin()).target() : Crag::Invalid);
		Crag::CragNode pv = (_crag.outArcs(v).size() > 0 ? (*_crag.outArcs(v).begin()).target() : Crag::Invalid);

		edgeFeatures.append(e, int(pu != Crag::Invalid && pu == pv));
		edgeFeatures.append(e, int(_crag.isRootNode(u) && _crag.isRootNode(v)));
		edgeFeatures.append(e, int(_crag.isLeafEdge(e)));
	}
}

void
FeatureExtractor::extractShapeEdgeFeatures(const NodeFeatures& nodeFeatures, EdgeFeatures& edgeFeatures) {

	LOG_DEBUG(featureextractorlog) << "extracting shape edge features..." << std::endl;
}

void
FeatureExtractor::extractVolumeRaysEdgeFeatures(EdgeFeatures& edgeFeatures) {

	LOG_DEBUG(featureextractorlog) << "extracting volume ray edge features..." << std::endl;

	VolumeRayFeature volumeRayFeature(_volumes, _rays);

	edgeFeatures.appendFeatureName(Crag::AdjacencyEdge, "mutual_piercing");
	edgeFeatures.appendFeatureName(Crag::AdjacencyEdge, "normalized_mutual_piercing");

	for (Crag::CragEdge e : _crag.edges()) {

		if (_crag.type(e) != Crag::AdjacencyEdge)
			continue;

		UTIL_TIME_SCOPE("extract volume rays edge features");

		// the longest piece of a ray from one node inside the other node
		util::ray<float, 3> uvRay;
		util::ray<float, 3> vuRay;
		double uvMaxPiercingDepth = volumeRayFeature.maxVolumeRayPiercingDepth(e.u(), e.v(), uvRay);
		double vuMaxPiercingDepth = volumeRayFeature.maxVolumeRayPiercingDepth(e.v(), e.u(), vuRay);

		// the largest mutual piercing distance
		double mutualPiercingScore = std::min(uvMaxPiercingDepth, vuMaxPiercingDepth);

		// normalize piercing dephts
		double norm = std::max(length(uvRay.direction()), length(vuRay.direction()));
		double normalizedMutualPiercingScore = mutualPiercingScore/norm;

		edgeFeatures.append(e, mutualPiercingScore);
		edgeFeatures.append(e, normalizedMutualPiercingScore);
	}
}

