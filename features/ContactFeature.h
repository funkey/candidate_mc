#ifndef CANDIDATE_MC_FEATURES_CONTACT_FEATURE_H__
#define CANDIDATE_MC_FEATURES_CONTACT_FEATURE_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>

/**
 * Implementation of the "contact" feature used in Gala.
 */
class ContactFeature {

public:

	ContactFeature(
			const Crag& crag,
			const CragVolumes& volumes,
			const ExplicitVolume<float>& boundaries,
			const std::vector<float>& thresholds = {0.1, 0.5, 0.9}) :
		_crag(crag),
		_volumes(volumes),
		_boundaries(boundaries),
		_thresholds(thresholds) {}

	std::vector<double> compute(Crag::CragEdge e);

private:

	std::vector<int> countVoxels(Crag::CragNode n);

	const Crag& _crag;
	const CragVolumes& _volumes;
	const ExplicitVolume<float> _boundaries;
	std::vector<float> _thresholds;
};

#endif // CANDIDATE_MC_FEATURES_CONTACT_FEATURE_H__

