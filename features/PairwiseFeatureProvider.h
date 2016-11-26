#ifndef CANDIDATE_MC_FEATURES_PAIRWISE_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_PAIRWISE_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"

class PairwiseFeatureProvider : public FeatureProvider<PairwiseFeatureProvider> {

public:

	PairwiseFeatureProvider(const Crag& crag, bool featureForEdges) :
		_crag(crag),
		_featureForEdges(featureForEdges){}

	template <typename ContainerT>
	void appendNodeFeatures(const Crag::CragNode n, ContainerT& adaptor) {

		auto type =_crag.type(n);
		auto iter = _nodeFeaturesNames.find(type);
		if (iter == _nodeFeaturesNames.end())
			for( unsigned int i = 0; i < adaptor.getFeatureNames(type).size(); i++)
				_nodeFeaturesNames[type].push_back( adaptor.getFeatureNames(type)[i] );

		// compute all pairwise products of all features and add them as well
		const std::vector<double>& features = adaptor.getFeatures();
		unsigned int numOriginalFeatures = features.size();

		for (unsigned int i = 0; i < numOriginalFeatures; i++)
			for (unsigned int j = i; j < numOriginalFeatures; j++)
				adaptor.append(features[i]*features[j]);
	}

	std::map<Crag::NodeType, std::vector<std::string>> getNodeFeatureNames() const override {

		std::map<Crag::NodeType, std::vector<std::string>> names;
		for (auto type : Crag::NodeTypes){

			std::vector<std::string> featureNames;
			auto iter = _nodeFeaturesNames.find(type);
			if (iter != _nodeFeaturesNames.end())
			{
				featureNames = iter->second;
				for (unsigned int i = 0; i < featureNames.size(); i++)
					for (unsigned int j = i; j < featureNames.size(); j++)
						names[type].push_back( featureNames[i] + "*" + featureNames[j]);
			}
		}

		return names;
	}

	template <typename ContainerT>
	void appendEdgeFeatures(const Crag::CragEdge e, ContainerT& adaptor) {

		if (!_featureForEdges)
			return;

		auto type =_crag.type(e);
		auto iter = _edgeFeaturesNames.find(type);
		if (iter == _edgeFeaturesNames.end()){
			for( unsigned int i = 0; i < adaptor.getFeatureNames(type).size(); i++)
				_edgeFeaturesNames[type].push_back( adaptor.getFeatureNames(type)[i] );
		}

		// compute all pairwise products of all features and add them as well
		const std::vector<double>& features = adaptor.getFeatures();
		unsigned int numOriginalFeatures = features.size();

		for (unsigned int i = 0; i < numOriginalFeatures; i++)
			for (unsigned int j = i; j < numOriginalFeatures; j++)
				adaptor.append(features[i]*features[j]);
	}

	std::map<Crag::EdgeType, std::vector<std::string>> getEdgeFeatureNames() const override {

		std::map<Crag::EdgeType, std::vector<std::string>> names;

		if (!_featureForEdges)
			return names;

		for (auto type : Crag::EdgeTypes){
			std::vector<std::string> featureNames;
			auto iter = _edgeFeaturesNames.find(type);
			if (iter != _edgeFeaturesNames.end())
			{
				featureNames = iter->second;
				for (unsigned int i = 0; i < featureNames.size(); i++)
					for (unsigned int j = i; j < featureNames.size(); j++)
						names[type].push_back( featureNames[i] + "*" + featureNames[j]);
			}
		}

		return names;
	}

private:
	const Crag& _crag;
	bool _featureForEdges;

	std::map<Crag::NodeType, std::vector<std::string>> _nodeFeaturesNames;
	std::map<Crag::EdgeType, std::vector<std::string>> _edgeFeaturesNames;
};

#endif // CANDIDATE_MC_FEATURES_PAIRWISE_FEATURE_PROVIDER_H__

