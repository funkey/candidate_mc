#ifndef CANDIDATE_MC_FEATURES_VOLUME_RAY_FEATURE_H__
#define CANDIDATE_MC_FEATURES_VOLUME_RAY_FEATURE_H__

#include <crag/CragVolumes.h>
#include "VolumeRays.h"

class VolumeRayFeature {

public:

	VolumeRayFeature(const CragVolumes& volumes, const VolumeRays& rays) : _volumes(volumes), _rays(rays) {}

	/**
	 * Determine the maximal piercing depth of any ray of node u into the volume 
	 * of node v. The maximal piercing ray is returned in maxPiercingRay.
	 */
	double maxVolumeRayPiercingDepth(Crag::CragNode u, Crag::CragNode v, util::ray<float, 3>& maxPiercingRay);

private:

	const CragVolumes& _volumes;
	const VolumeRays&  _rays;
};

#endif // CANDIDATE_MC_FEATURES_VOLUME_RAY_FEATURE_H__

