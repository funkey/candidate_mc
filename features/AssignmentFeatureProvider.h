#ifndef CANDIDATE_MC_FEATURES_ASSIGNMENT_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_ASSIGNMENT_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"
#include "HausdorffDistance.h"
#include "Overlap.h"
#include <util/helpers.hpp>

class AssignmentFeatureProvider : public FeatureProvider<AssignmentFeatureProvider> {

public:

	struct Parameters {

		Parameters() :
			affinitiesPositiveDirection(true),
			maxHausdorffDistance(100) {}

		/**
		 * If true, affinity values of voxel (x,y,z) are treated as affinities 
		 * into the positive direction (e.g., to voxel (x,y,z+1)).
		 */
		bool affinitiesPositiveDirection;

		/**
		 * Clip Hausdorff distance values above this threshold.
		 */
		double maxHausdorffDistance;
	};

	AssignmentFeatureProvider(
			const Crag& crag,
			const CragVolumes& volumes,
			const ExplicitVolume<float>& affinitiesZ,
			const NodeFeatures& nodeFeatures,
			const Parameters& parameters = Parameters()) :

		_crag(crag),
		_volumes (volumes),
		_affs(affinitiesZ),
		_features(nodeFeatures),
		_hausdorff(parameters.maxHausdorffDistance),
		_sizeFeatureIndex(-1),
		_parameters(parameters) {}

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

		adaptor.append(getHausdorffDistance(u, v));

		std::vector<double> features = getAffinityFeatures(u, v);
		for (unsigned int i = 0; i < features.size(); i++)
			adaptor.append(features[i]);

		// first affinity feature is number of contact voxels, i.e., overlap
		int overlap = features[0];
		int size_u = getSize(u);
		int size_v = getSize(v);
		int setDifference = size_u + size_v - 2*overlap;

		adaptor.append(std::abs(size_u - size_v));
		adaptor.append(setDifference);
	}

	std::map<Crag::NodeType, std::vector<std::string>> getNodeFeatureNames() const override {

		std::map<Crag::NodeType, std::vector<std::string>> names;

		names[Crag::AssignmentNode].push_back("hausdorff distance");
		names[Crag::AssignmentNode].push_back("overlap");
		names[Crag::AssignmentNode].push_back("affinity min");
		names[Crag::AssignmentNode].push_back("affinity median");
		names[Crag::AssignmentNode].push_back("affinity max");
		names[Crag::AssignmentNode].push_back("size difference");
		names[Crag::AssignmentNode].push_back("set difference");

		return names;
	}

private:

	inline double getHausdorffDistance(Crag::CragNode i, Crag::CragNode j) {

		double i_j, j_i;
		_hausdorff(*_volumes[i], *_volumes[j], i_j, j_i);

		return std::max(i_j, j_i);
	}

	inline double getSize(Crag::CragNode n) {

		UTIL_ASSERT_REL(_crag.type(n), ==, Crag::SliceNode);

		if (_sizeFeatureIndex == -1)
			findSizeFeature();

		return _features[n][_sizeFeatureIndex];
	}

	void findSizeFeature() {

		auto it = std::find(
				_features.getFeatureNames(Crag::SliceNode).begin(),
				_features.getFeatureNames(Crag::SliceNode).end(),
				"membranes size");

		if (it == _features.getFeatureNames(Crag::SliceNode).end())
			UTIL_THROW_EXCEPTION(
					UsageError,
					"Feature 'membranes size' (the size of a node) was not computed prior to running AssignmentFeatureProvider. "
					"Encountered features: " << _features.getFeatureNames(Crag::SliceNode)
			);

		_sizeFeatureIndex = it - _features.getFeatureNames(Crag::SliceNode).begin();
	}

	double differences(Crag::CragNode i, double overlap) {

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

	std::vector<double> getAffinityFeatures(Crag::CragNode i, Crag::CragNode j) {

		UTIL_ASSERT_REL(_crag.type(i), ==, Crag::SliceNode);
		UTIL_ASSERT_REL(_crag.type(j), ==, Crag::SliceNode);

		// make sure i is lower in z
		if (_volumes[i]->getBoundingBox().center().z() > _volumes[j]->getBoundingBox().center().z())
			std::swap(i, j);

		// list of voxel affinity values between the two slice nodes
		std::vector<float> contactAffinities;

		const CragVolume& vol_i = *_volumes[i];
		const CragVolume& vol_j = *_volumes[j];

		util::point<int,3> discreteGlobalOffset_i = vol_i.getOffset()/vol_i.getResolution();
		util::point<int,3> discreteGlobalOffset_j = vol_j.getOffset()/vol_j.getResolution();

		// offset to add to 2D locations in i to get to 2D locations in j
		util::point<int,3> discreteOffset_i_to_j  = discreteGlobalOffset_j - discreteGlobalOffset_i;
		discreteOffset_i_to_j.z() = 0;

		// If affinities point in the positive axis directions, we have to read 
		// the values in slice i, otherwise in j. This offset handles that.
		int affinityZIndex = (_parameters.affinitiesPositiveDirection ? discreteGlobalOffset_i.z() : discreteGlobalOffset_j.z());

		for (int y = 0; y < vol_i.height(); y++)
		for (int x = 0; x < vol_i.width();  x++) {

			// 2D position inside volume i
			util::point<int,3> pos_i(x, y, 0);

			if (!vol_i[pos_i])
				// is not part of candidate i
				continue;

			// same 2D position inside volume j
			util::point<int,3> pos_j = pos_i - discreteOffset_i_to_j;

			if (!vol_j.getDiscreteBoundingBox().contains(pos_j))
				// does not overlap with vol_j
				continue;

			if (!vol_j[pos_j])
				// is not part of candidate j
				continue;

			// global 3D position
			util::point<int,3> globalDiscretePosition = discreteGlobalOffset_i + pos_i;
			util::point<int,3> affPos(
					globalDiscretePosition.x(),
					globalDiscretePosition.y(),
					affinityZIndex);

			contactAffinities.push_back(_affs[affPos]);
		}

		if (contactAffinities.size() == 0)
			return {0, 0, 0, 0};

		std::vector<double> features;

		std::sort(contactAffinities.begin(), contactAffinities.end());
		// number of contact voxels
		features.push_back(contactAffinities.size());
		// min
		features.push_back(*contactAffinities.begin());
		// median
		features.push_back(*(contactAffinities.begin() + contactAffinities.size()/2));
		// max
		features.push_back(*contactAffinities.rbegin());

		return features;
	}

	const Crag& _crag;
	const CragVolumes& _volumes;
	const ExplicitVolume<float>& _affs;

	// already extracted features
	const NodeFeatures& _features;

	HausdorffDistance _hausdorff;
	Overlap _overlap;

	int _sizeFeatureIndex;

	Parameters _parameters;
};

#endif // CANDIDATE_MC_FEATURES_ASSIGNMENT_FEATURE_PROVIDER_H__

