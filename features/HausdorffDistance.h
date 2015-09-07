#ifndef CANDIDATE_MC_FEATURES_HAUSDORFF_DISTANCE_H__
#define CANDIDATE_MC_FEATURES_HAUSDORFF_DISTANCE_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>

/**
 * Computes the HausdorffDistance of pairs of CRAG volumes. Ignores the 
 * z-dimension and assumes the volumes to the nodes have a depth of 1. The 
 * functor has an internal cache that relies on the addresses of the given 
 * CragVolume objects, so be aware that re-allocation of your volumes can 
 * invalidate the cache. The cache can be cleared with a call to clearCache(), 
 * which is also usefull to free up memory.
 */
class HausdorffDistance {

public:

	/**
	 * Create a new functor that can compute the Hausdorff distance for pairs of 
	 * CragVolumes.
	 *
	 * @param maxDistance
	 *              The maximal Hausdorff distance to be reported. If two 
	 *              volumes exceed this value, this is the value that will be 
	 *              reported.
	 */
	HausdorffDistance(int maxDistance);

	/**
	 * Compute the distances for volumes i and j. Results are returned in 
	 * reference i_j (distance of node i to j) and j_i (vice versa).
	 */
	void operator()(const CragVolume& i, const CragVolume& j, double& i_j, double& j_i);

	/**
	 * Free memory allocated for the cache.
	 */
	void clearCache() { _cache.clear(); _distanceMaps.clear(); }

private:

	void volumesDistance(
			const CragVolume& volume_i,
			const CragVolume& volume_j,
			double& i_j);

	// lower bound HausdorffDistance between a and b based on bounding boxes
	double lowerBound(const CragVolume& a, const CragVolume& b);

	vigra::MultiArray<2, double>& getDistanceMap(const CragVolume& volume);

	std::map<std::pair<const CragVolume*, const CragVolume*>, std::pair<double, double>> _cache;

	std::map<const CragVolume*, vigra::MultiArray<2, double>> _distanceMaps;

	double _maxDistance;

	int _padX;
	int _padY;
};

#endif // CANDIDATE_MC_FEATURES_HAUSDORFF_DISTANCE_H__

