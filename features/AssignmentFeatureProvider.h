#ifndef CANDIDATE_MC_FEATURES_ASSIGNMENT_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_ASSIGNMENT_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"
#include "HausdorffDistance.h"
#include "Overlap.h"

class AssignmentFeatureProvider : public FeatureProvider<AssignmentFeatureProvider> {

public:

	AssignmentFeatureProvider(
			const Crag& crag,
			const CragVolumes& volumes,
			const ExplicitVolume<float>& affinitiesZ,
			const NodeFeatures& nodeFeatures,
			double maxHausdorffDistance = 100) :

		_crag(crag),
		_volumes (volumes),
		_affs(affinitiesZ),
		_features(nodeFeatures),
		_hausdorff(maxHausdorffDistance),
		_sizeFeatureIndex(-1) {}

	template <typename ContainerT>
	void appendNodeFeatures(const Crag::CragNode n, ContainerT& adaptor) {

		if (_crag.type(n) != Crag::AssignmentNode)
			return;

		if (_crag.inArcs(n).size() != 2)
			UTIL_THROW_EXCEPTION(
					UsageError,
					"assignment nodes with more than two slice nodes not yet supported"
			);

		Crag::CragNode u = (*_crag.inArcs(n).begin()).source();
		Crag::CragNode v = (*(_crag.inArcs(n).begin()++)).source();

		adaptor.append(hausdorffDistance(u, v));
		adaptor.append(overlap(u, v));
		adaptor.append(sizeDifference(u, v));
	}

	std::map<Crag::NodeType, std::vector<std::string>> getNodeFeatureNames() const override {

		std::map<Crag::NodeType, std::vector<std::string>> names;

		names[Crag::AssignmentNode].push_back("hausdorff distance");
		names[Crag::AssignmentNode].push_back("overlap");
		names[Crag::AssignmentNode].push_back("size difference");

		return names;
	}

private:

	inline double hausdorffDistance(Crag::CragNode i, Crag::CragNode j) {

		double i_j, j_i;
		_hausdorff(*_volumes[i], *_volumes[j], i_j, j_i);

		return std::max(i_j, j_i);
	}

	inline double overlap(Crag::CragNode i, Crag::CragNode j) {

		return _overlap(*_volumes[i], *_volumes[j]);
	}

	inline double sizeDifference(Crag::CragNode i, Crag::CragNode j) {

		if (_sizeFeatureIndex == -1)
			findSizeFeature();

		double size_i = _features[i][_sizeFeatureIndex];
		double size_j = _features[j][_sizeFeatureIndex];

		return std::abs(size_i - size_j);
	}

	void findSizeFeature() {

		auto it = std::find(
				_features.getFeatureNames(Crag::AssignmentNode).begin(),
				_features.getFeatureNames(Crag::AssignmentNode).end(),
				"membrane size");

		if (it == _features.getFeatureNames(Crag::AssignmentNode).end())
			UTIL_THROW_EXCEPTION(
					UsageError,
					"feature 'membrane size' (the size of a node) was not computed prior to running AssignmentFeatureProvider"
			);

		_sizeFeatureIndex = it - _features.getFeatureNames(Crag::AssignmentNode).begin();
	}

	const Crag& _crag;
	const CragVolumes& _volumes;
	const ExplicitVolume<float>& _affs;

	// already extracted features
	const NodeFeatures& _features;

	HausdorffDistance _hausdorff;
	Overlap _overlap;

	int _sizeFeatureIndex;
};

#endif // CANDIDATE_MC_FEATURES_ASSIGNMENT_FEATURE_PROVIDER_H__

