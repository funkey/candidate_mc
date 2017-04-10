#ifndef CANDIDATE_MC_FEATURES_MERGE_TREE_SCORE_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_MERGE_TREE_SCORE_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"
#include "io/CragImport.h"

class MergeTreeScoreFeatureProvider : public FeatureProvider<MergeTreeScoreFeatureProvider> {

public:

	MergeTreeScoreFeatureProvider(
			const Crag& crag,
			const CragVolumes& volumes,
			const Crag::NodeMap<int>& nodeToId,
			std::string mergeHistoryPath) :
		_crag (crag),
		_volumes (volumes),
		_nodeToId (nodeToId),
		_mergeHistoryPath (mergeHistoryPath){}

	template <typename ContainerT>
	void appendEdgeFeatures(const Crag::CragEdge e, ContainerT& adaptor) {

		if (_crag.type(e) == Crag::AdjacencyEdge)
		{
//			Crag::CragNode u = e.u();
//			Crag::CragNode v = e.v();

//			int uMergeId = _nodeToId[u];
//			int vMergeId = _nodeToId[v];

		}
	}

	std::map<Crag::EdgeType, std::vector<std::string>> getEdgeFeatureNames() const override {

		std::map<Crag::EdgeType, std::vector<std::string>> names;
		names[Crag::AdjacencyEdge].push_back("merge_score");
		names[Crag::AdjacencyEdge].push_back("smallest_path");
		names[Crag::AdjacencyEdge].push_back("longest_path");
		return names;
	}

private:

	const Crag& _crag;
	const CragVolumes& _volumes;
	const Crag::NodeMap<int>& _nodeToId;

	std::string _mergeHistoryPath;
};

#endif // CANDIDATE_MC_FEATURES_MERGE_TREE_SCORE_FEATURE_PROVIDER_H__

