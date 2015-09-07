#ifndef CANDIDATE_MC_FEATURES_OVERLAP_H__
#define CANDIDATE_MC_FEATURES_OVERLAP_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>

class Overlap {

public:

	/**
	 * Get the volume of the overlap of non-zero voxels between a and b. This 
	 * respects the resolution of the voxels.
	 */
	double operator()(const CragVolume& a, const CragVolume& b);

	/**
	 * Check if the overlap between a and b exceeds the given value. This method 
	 * is usually faster then computing the exact overlap.
	 */
	bool exceeds(const CragVolume& a, CragVolume& b, double value);
};

#endif // CANDIDATE_MC_FEATURES_OVERLAP_H__

