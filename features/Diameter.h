#ifndef CANDIDATE_MC_FEATURES_DIAMETER_H__
#define CANDIDATE_MC_FEATURES_DIAMETER_H__

#include <imageprocessing/ExplicitVolume.h>
#include <crag/CragVolume.h>

/**
 * Computes the diameter of 2D nodes. Ignores the z-dimension and assumes the 
 * volumes to the nodes have a depth of 1.
 */
class Diameter {

public:

	/**
	 * Compute the diameter for a CragVolume.
	 */
	double operator()(const CragVolume& volume);
};

#endif // CANDIDATE_MC_FEATURES_DIAMETER_H__

