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
		names[Crag::VolumeNode].push_back("level");
		names[Crag::VolumeNode].push_back("num descendants");

		return names;
	}

private:

	std::pair<int, int> recExtractTopologicalFeatures(Crag::CragNode n) {

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

	Crag::NodeMap<std::vector<double>> _features;
};

#endif // CANDIDATE_MC_FEATURES_TOPOLOGICAL_FEATURE_PROVIDER_H__

