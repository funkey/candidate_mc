#ifndef CANDIDATE_MC_FEATURES_HAUSDORFF_DISTANCE_H__
#define CANDIDATE_MC_FEATURES_HAUSDORFF_DISTANCE_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>

/**
 * Computes the HausdorffDistance of pairs of CRAG nodes. Ignores the 
 * z-dimension and assumes the volumes to the nodes have a depth of 1.
 */
class HausdorffDistance {

public:

	/**
	 * Create a new functor that can compute the Hausdorff distance for every 
	 * pair of nodes for which volumes a and b were given.
	 *
	 * @param a
	 *              The first set of volumes.
	 *
	 * @param b
	 *              The second set of volumes.
	 *
	 * @param maxDistance
	 *              The maximal Hausdorff distance to be reported. If two 
	 *              volumes exceed this value, this is the value that will be 
	 *              reported.
	 */
	HausdorffDistance(const CragVolumes& a, const CragVolumes& b, int maxDistance);

	/**
	 * Compute the distances for nodes i (of CRAG a) and node j (of CRAG b). 
	 * Results are returned in reference i_j (distance of node i to j) and j_i 
	 * (vice versa).
	 */
	void operator()(Crag::Node i, Crag::Node j, double& i_j, double& j_i);

private:

	// map of each node to all contained leaf nodes
	typedef std::map<Crag::Node, std::vector<Crag::Node>> LeafNodeMap;

	LeafNodeMap collectLeafNodes(const Crag& crag);

	void leafDistance(
			Crag::Node i,
			Crag::Node j,
			double& i_j,
			double& j_i);

	void volumesDistance(
			std::shared_ptr<CragVolume> volume_i,
			std::shared_ptr<CragVolume> volume_j,
			double& i_j);

	vigra::MultiArray<2, double>& getDistanceMap(std::shared_ptr<CragVolume> volume);

	void recCollectLeafNodes(const Crag& crag, Crag::Node n, LeafNodeMap& map);

	LeafNodeMap _leafNodesA;
	LeafNodeMap _leafNodesB;

	const CragVolumes& _a;
	const CragVolumes& _b;

	std::map<std::pair<Crag::Node, Crag::Node>, std::pair<double, double>> _cache;

	std::map<std::shared_ptr<CragVolume>, vigra::MultiArray<2, double>> _distanceMaps;

	int _maxDistance;
};

#endif // CANDIDATE_MC_FEATURES_HAUSDORFF_DISTANCE_H__

