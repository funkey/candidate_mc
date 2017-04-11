#ifndef CANDIDATE_MC_FEATURES_MERGE_TREE_SCORE_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_MERGE_TREE_SCORE_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"
#include "io/CragImport.h"
#include <iterator>

class MergeTreeScoreFeatureProvider : public FeatureProvider<MergeTreeScoreFeatureProvider> {

public:

	MergeTreeScoreFeatureProvider(
			const Crag& crag,
			const CragVolumes& volumes,
			const Crag::NodeMap<int>& nodeToId,
			const Costs& mergeCosts,
			std::string mergeHistoryPath) :
		_crag (crag),
		_volumes (volumes),
		_nodeToId (nodeToId),
		_mergeCosts (mergeCosts),
		_mergeHistoryPath (mergeHistoryPath){}

	template <typename ContainerT>
	void appendEdgeFeatures(const Crag::CragEdge e, ContainerT& adaptor) {

		if (_crag.type(e) == Crag::AdjacencyEdge)
		{
			std::cout << "merge tree score feature" << std::endl;

			Crag::CragNode u = e.u();
			Crag::CragNode v = e.v();

			std::string s = "-";
			std::vector<Crag::CragNode> uParents;
			std::cout << "child id: " << _crag.id(u) << std::endl;
			getParents(u, uParents, s);
			std::cout << "child id: " << _crag.id(v) << std::endl;
			std::vector<Crag::CragNode> vParents;
			getParents(v, vParents, s);

			std::cout << "set of parents from u: " << std::endl;
			std::vector<int> uId;
			for(int i=0; i < uParents.size(); i++) {
				std::cout << _crag.id(uParents[i]) << std::endl;
				uId.push_back(_crag.id(uParents[i]));
			}

			std::cout << "set of parents from v: " << std::endl;
			std::vector<int> vId;
			for(int i=0; i < vParents.size(); i++) {
				std::cout << _crag.id(vParents[i]) << std::endl;
				vId.push_back(_crag.id(vParents[i]));
			}

			std::cout << "intersection " << std::endl;
			std::vector<Crag::CragNode> commonElements;
			std::set_intersection (uParents.begin(),uParents.end(),vParents.begin(),vParents.end(),
					std::back_inserter(commonElements));

			if (commonElements.size() > 0)
				std::cout << "Elements in common" << std::endl;

			for (int i = 0; i < commonElements.size(); i++)
			{
				std::cout << _crag.id(commonElements[i]) << std::endl;
			}

			Crag::CragNode closestParent;
			if (commonElements.size() > 0){
				closestParent = commonElements[commonElements.size()-1];

				int mergeId = _nodeToId[closestParent];
				std::cout << " merge tree id of node: " << _crag.id(closestParent) << " is: " << mergeId << std::endl;
				std::cout << "cost of merged node: " << _mergeCosts.node[closestParent] << std::endl;
				adaptor.append(_mergeCosts.node[closestParent]);

				std::vector<int>::iterator it;
				std::sort(uId.begin(), uId.end());
				std::sort(vId.begin(), vId.end());
				it = std::find (uId.begin(), uId.end(), _crag.id(closestParent));
				int uDistance = 0, vDistance = 0;
				if (it != uId.end())
				{
					uDistance = std::distance(uId.begin(), it) + 1;
					std::cout << "distance from u to common closest parent is: " << std::distance(uId.begin(), it) << std::endl;
				}

				it = std::find (vId.begin(), vId.end(), _crag.id(closestParent));
				if (it != vId.end())
				{
					vDistance = std::distance(vId.begin(), it) + 1;
					std::cout << "distance from v to common closest parent is: " << std::distance(vId.begin(), it) << std::endl;
				}

				// shortest path
				adaptor.append( (uDistance <= vDistance) ? uDistance : vDistance);
				// longest path
				adaptor.append( (uDistance > vDistance) ? uDistance : vDistance);
			}
			else{
				// if there is no elements in common, score, shortest and longest path are defined as the maximum int.
				adaptor.append(INT_MAX);
				adaptor.append(INT_MAX);
				adaptor.append(INT_MAX);
			}
		}
	}

	std::map<Crag::EdgeType, std::vector<std::string>> getEdgeFeatureNames() const override {

		std::map<Crag::EdgeType, std::vector<std::string>> names;
		names[Crag::AdjacencyEdge].push_back("merge_score");
		names[Crag::AdjacencyEdge].push_back("shortest_path");
		names[Crag::AdjacencyEdge].push_back("longest_path");
		return names;
	}

private:

	void getParents( Crag::CragNode n, std::vector<Crag::CragNode>& parents, std::string s) {
		// for each parent
		for (Crag::CragArc a : _crag.outArcs(n)){
			Crag::CragNode parent = a.target();
			std::cout << s << "parent id: " << _crag.id(parent) << std::endl;
			s += "-";
			getParents(parent, parents, s);

			parents.push_back(parent);
		}
	}

	const Crag& _crag;
	const CragVolumes& _volumes;
	const Crag::NodeMap<int>& _nodeToId;
	const Costs& _mergeCosts;

	std::string _mergeHistoryPath;
};

#endif // CANDIDATE_MC_FEATURES_MERGE_TREE_SCORE_FEATURE_PROVIDER_H__

