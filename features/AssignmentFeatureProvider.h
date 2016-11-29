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

		Crag::CragNode u = (*(_crag.inArcs(n).begin())).source();
		Crag::CragNode v = (*(++_crag.inArcs(n).begin())).source();

		adaptor.append(hausdorffDistance(u, v));
		double value = overlap(u, v);
		adaptor.append(value);
		adaptor.append(sizeDifference(u, v));
		adaptor.append(setDifference(u, value));
		adaptor.append(setDifference(v, value));


	}

	std::map<Crag::NodeType, std::vector<std::string>> getNodeFeatureNames() const override {

		std::map<Crag::NodeType, std::vector<std::string>> names;

		names[Crag::AssignmentNode].push_back("hausdorff distance");
		names[Crag::AssignmentNode].push_back("overlap");
		names[Crag::AssignmentNode].push_back("size difference");
		names[Crag::AssignmentNode].push_back("set difference u");
		names[Crag::AssignmentNode].push_back("set difference v");

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
				"membranes boundary size");

		if (it == _features.getFeatureNames(Crag::AssignmentNode).end())
			UTIL_THROW_EXCEPTION(
					UsageError,
					"feature 'membranes boundary size' (the size of a node) was not computed prior to running AssignmentFeatureProvider"
			);

		_sizeFeatureIndex = it - _features.getFeatureNames(Crag::AssignmentNode).begin();
	}

	double setDifference(Crag::CragNode i, double overlap){

		CragVolume& vol_i = *_volumes[i];

		double totalVolume = 0.0;
		for (int z = 0; z < vol_i.depth();  z++)
		for (int y = 0; y < vol_i.height(); y++)
		for (int x = 0; x < vol_i.width();  x++) {

			if (vol_i(x, y, z) == 0)
				continue;

			totalVolume += (vol_i.getResolution().x() * vol_i.getResolution().y() *
							vol_i.getResolution().z());
		}

		return totalVolume - overlap;
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

