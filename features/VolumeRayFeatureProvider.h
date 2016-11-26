#ifndef CANDIDATE_MC_FEATURES_VOLUME_RAY_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_VOLUME_RAY_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"
#include "VolumeRayFeature.h"

#include <features/VolumeRays.h>
#include <util/geometry.hpp>

class VolumeRayFeatureProvider : public FeatureProvider<VolumeRayFeatureProvider> {

public:

	VolumeRayFeatureProvider(
			const Crag& crag,
			const CragVolumes& volumes,
			const VolumeRays& rays) :
		_crag(crag),
		_volumes (volumes),
		_rays(rays){}

	template <typename ContainerT>
	void appendEdgeFeatures(const Crag::CragEdge e, ContainerT& adaptor) {

		if (_crag.type(e) == Crag::AdjacencyEdge)
		{
			VolumeRayFeature volumeRayFeature(_volumes, _rays);

			// the longest piece of a ray from one node inside the other node
			util::ray<float, 3> uvRay;
			util::ray<float, 3> vuRay;
			double uvMaxPiercingDepth = volumeRayFeature.maxVolumeRayPiercingDepth(e.u(), e.v(), uvRay);
			double vuMaxPiercingDepth = volumeRayFeature.maxVolumeRayPiercingDepth(e.v(), e.u(), vuRay);

			// the largest mutual piercing distance
			double mutualPiercingScore = std::min(uvMaxPiercingDepth, vuMaxPiercingDepth);

			// normalize piercing dephts
			double norm = std::max(length(uvRay.direction()), length(vuRay.direction()));
			double normalizedMutualPiercingScore = mutualPiercingScore/norm;

			adaptor.append(mutualPiercingScore);
			adaptor.append(normalizedMutualPiercingScore);
		}
	}

	std::map<Crag::EdgeType, std::vector<std::string>> getEdgeFeatureNames() const override {

		std::map<Crag::EdgeType, std::vector<std::string>> names;

		names[Crag::AdjacencyEdge].push_back("mutual_piercing");
		names[Crag::AdjacencyEdge].push_back("normalized_mutual_piercing");

		return names;
	}

private:

	const Crag&        _crag;
	const CragVolumes& _volumes;
	const VolumeRays&  _rays;
};

#endif // CANDIDATE_MC_FEATURES_VOLUME_RAY_FEATURE_PROVIDER_H__

