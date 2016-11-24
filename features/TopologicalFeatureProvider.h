#ifndef CANDIDATE_MC_FEATURES_TOPOLOGICAL_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_TOPOLOGICAL_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"

class TopologicalFeatureProvider : public FeatureProvider<TopologicalFeatureProvider> {

public:

	TopologicalFeatureProvider(const Crag& crag) :
		_crag(crag),
		_features(crag) {

		for (Crag::CragNode n : _crag.nodes())
			if (_crag.isRootNode(n) && _crag.type(n) != Crag::NoAssignmentNode)
				recExtractTopologicalFeatures(n);
	}

	template <typename ContainerT>
	void appendNodeFeatures(const Crag::CragNode n, ContainerT& adaptor) {

		for (double feature : _features[n])
			adaptor.append(feature);
	}

	std::map<Crag::NodeType, std::vector<std::string>> getNodeFeatureNames() const override {

		std::map<Crag::NodeType, std::vector<std::string>> names;

		names[Crag::SliceNode].push_back("level");
		names[Crag::SliceNode].push_back("num descendants");
		//TODO: AssignmentNode && NoAssignmentNode?
		names[Crag::VolumeNode].push_back("level");
		names[Crag::VolumeNode].push_back("num descendants");

		return names;
	}

	template <typename ContainerT>
	void appendEdgeFeatures(const Crag::CragEdge e, ContainerT& adaptor) {

		Crag::CragNode u = e.u();
		Crag::CragNode v = e.v();

		adaptor.append(std::min(_crag.getLevel(u), _crag.getLevel(v)));
		adaptor.append(std::max(_crag.getLevel(u), _crag.getLevel(v)));

		Crag::CragNode pu = (_crag.outArcs(u).size() > 0 ? (*_crag.outArcs(u).begin()).target() : Crag::Invalid);
		Crag::CragNode pv = (_crag.outArcs(v).size() > 0 ? (*_crag.outArcs(v).begin()).target() : Crag::Invalid);

		adaptor.append(int(pu != Crag::Invalid && pu == pv));
		adaptor.append(int(_crag.isRootNode(u) && _crag.isRootNode(v)));
		adaptor.append(int(_crag.isLeafEdge(e)));

	}

	std::map<Crag::EdgeType, std::vector<std::string>> getEdgeFeatureNames() const override {

		std::map<Crag::EdgeType, std::vector<std::string>> names;

		for (auto type : Crag::EdgeTypes) {

			names[type].push_back("min_level");
			names[type].push_back("max_level");
			names[type].push_back("siblings");
			names[type].push_back("root_edge");
			names[type].push_back("leaf_edge");
		}

		return names;
	}

private:

	std::pair<int, int> recExtractTopologicalFeatures(Crag::CragNode n) {

		if (!_features[n].empty())
			return std::make_pair(_features[n][0], _features[n][1]);

		int numChildren    = 0;
		int numDescendants = 0;
		int level          = 1; // level of leaf nodes

		for (Crag::CragArc e : _crag.inArcs(n)) {

			std::pair<int, int> level_descendants =
					recExtractTopologicalFeatures(e.source());

			level = std::max(level, level_descendants.first + 1);
			numDescendants += level_descendants.second;
			numChildren++;
		}

		numDescendants += numChildren;

		_features[n].push_back(level);
		_features[n].push_back(numDescendants);

		return std::make_pair(level, numDescendants);
	}

	const Crag& _crag;

	Crag::NodeMap<std::vector<int>> _features;
};

#endif // CANDIDATE_MC_FEATURES_TOPOLOGICAL_FEATURE_PROVIDER_H__

