#include <fstream>
#include <sstream>
#include <io/vectors.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/helpers.hpp>
#include <util/timing.h>
#include <util/assert.h>
#include "FeatureExtractor.h"
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

// FEATURE NORMALIZATION AND POST-PROCESSING
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

	// append a 1 for bias
	for (Crag::CragNode n : _crag.nodes())
		if (_crag.type(n) != Crag::NoAssignmentNode)
			nodeFeatures.append(n, 1);

	for (auto type : Crag::NodeTypes)
		if (type != Crag::NoAssignmentNode)
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

	// append a 1 for bias
	for (Crag::CragEdge e : _crag.edges())
		if (_crag.type(e) != Crag::AssignmentEdge && _crag.type(e) != Crag::SeparationEdge)
			edgeFeatures.append(e, 1);


	for (auto type : Crag::EdgeTypes)
		if (type != Crag::AssignmentEdge && type != Crag::SeparationEdge)
				edgeFeatures.appendFeatureName(type, "bias");

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
