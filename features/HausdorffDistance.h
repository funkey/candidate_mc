#ifndef CANDIDATE_MC_FEATURES_HAUSDORFF_DISTANCE_H__
#define CANDIDATE_MC_FEATURES_HAUSDORFF_DISTANCE_H__

#include <crag/Crag.h>

/**
 * Computes the HausdorffDistance of pairs of CRAG nodes. Ignores the 
 * z-dimension and assumes the volumes to the nodes have a depth of 1.
 */
class HausdorffDistance {

public:

	/**
	 * Create a new HausdorffDistance functor for nodes of the given CRAGs.
	 */
	HausdorffDistance(const Crag& a, const Crag& b, int maxDistance);

	/**
	 * Compute the distances for nodes i (of CRAG a) and node j (of CRAG b). 
	 * Results are returned in reference i_j (distance of node i to j) and j_i 
	 * (vice versa).
	 */
	void distance(Crag::Node i, Crag::Node j, double& i_j, double& j_i);

private:

	// map of each node to all contained leaf nodes
	typedef std::map<Crag::Node, std::vector<Crag::Node>> LeafNodeMap;

	LeafNodeMap collectLeafNodes(const Crag& crag);

	void leafDistance(Crag::Node i, Crag::Node j, double& i_j, double& j_i);

	void leafDistance(
			const ExplicitVolume<unsigned char>& volume_i,
			const ExplicitVolume<unsigned char>& volume_j,
			double& i_j);

	vigra::MultiArray<2, double>& getDistanceMap(const ExplicitVolume<unsigned char>& volume);

	void recCollectLeafNodes(const Crag& crag, Crag::Node n, LeafNodeMap& map);

	LeafNodeMap _leafNodesA;
	LeafNodeMap _leafNodesB;

	const Crag& _a;
	const Crag& _b;

	std::map<std::pair<Crag::Node, Crag::Node>, std::pair<double, double>> _cache;

	std::map<const ExplicitVolume<unsigned char>*, vigra::MultiArray<2, double>> _distanceMaps;

	int _maxDistance;
};

#endif // CANDIDATE_MC_FEATURES_HAUSDORFF_DISTANCE_H__

