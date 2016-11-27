#ifndef CANDIDATE_MC_FEATURES_BIAS_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_BIAS_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"

class BiasFeatureProvider : public FeatureProvider<BiasFeatureProvider> {

public:

	BiasFeatureProvider(
			const Crag& crag,
			NodeFeatures& nodeFeatures,
			EdgeFeatures& edgeFeatures) :
		_crag(crag),
		_nodeFeatures(nodeFeatures),
		_edgeFeatures(edgeFeatures){

	}

	template <typename ContainerT>
	void appendNodeFeatures(const Crag::CragNode n, ContainerT& adaptor) {

		if (_crag.type(n) != Crag::NoAssignmentNode)
			adaptor.append(1);
	}

	std::map<Crag::NodeType, std::vector<std::string>> getNodeFeatureNames() const override {

		std::map<Crag::NodeType, std::vector<std::string>> names;

		for (auto type : Crag::NodeTypes)
			if (type != Crag::NoAssignmentNode)
				names[type].push_back("bias");

		return names;
	}

	template <typename ContainerT>
	void appendEdgeFeatures(const Crag::CragEdge e, ContainerT& adaptor) {

		if (_crag.type(e) != Crag::AssignmentEdge && _crag.type(e) != Crag::SeparationEdge)
			adaptor.append(1);
	}

	std::map<Crag::EdgeType, std::vector<std::string>> getEdgeFeatureNames() const override {

		std::map<Crag::EdgeType, std::vector<std::string>> names;

		for (auto type : Crag::EdgeTypes)
			if (type != Crag::AssignmentEdge && type != Crag::SeparationEdge)
				names[type].push_back("bias");

		return names;
	}

private:
	const Crag& _crag;

	// already extracted features
	NodeFeatures& _nodeFeatures;
	EdgeFeatures& _edgeFeatures;
};

#endif // CANDIDATE_MC_FEATURES_BIAS_FEATURE_PROVIDER_H__

