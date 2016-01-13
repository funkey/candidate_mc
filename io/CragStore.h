#ifndef TREE_MC_IO_CRAG_STORE_H__
#define TREE_MC_IO_CRAG_STORE_H__

#include <set>
#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <features/NodeFeatures.h>
#include <features/EdgeFeatures.h>
#include <features/Skeletons.h>
#include <features/VolumeRays.h>
#include <inference/Costs.h>
#include <inference/CragSolution.h>

/**
 * Interface definition for crag stores.
 */
class CragStore {

public:

	virtual ~CragStore() {}

	/**
	 * Store a candidate region adjacency graph (CRAG).
	 */
	virtual void saveCrag(const Crag& crag) = 0;

	/**
	 * Save CRAG volumes. This will only store the volumes of leaf nodes, others 
	 * can be assembled from them.
	 */
	virtual void saveVolumes(const CragVolumes& volumes) = 0;

	/**
	 * Store features for the candidates (i.e., the nodes) of a CRAG.
	 */
	virtual void saveNodeFeatures(const Crag& crag, const NodeFeatures& features) = 0;

	/**
	 * Store features for adjacent candidates (i.e., the edges) of a CRAG.
	 */
	virtual void saveEdgeFeatures(const Crag& crag, const EdgeFeatures& features) = 0;

	/**
	 * Store the min and max values of the node features.
	 */
	virtual void saveFeaturesMin(const FeatureWeights& min) = 0;
	virtual void saveFeaturesMax(const FeatureWeights& max) = 0;

	/**
	 * Store the skeletons for candidates of a CRAG.
	 */
	virtual void saveSkeletons(const Crag& crag, const Skeletons& skeletons) = 0;

	/**
	 * Store the volume rays for candidates of a CRAG.
	 */
	virtual void saveVolumeRays(const VolumeRays& rays) = 0;

	/**
	 * Store a set of feature weights.
	 */
	virtual void saveFeatureWeights(const FeatureWeights& weights) = 0;

	/**
	 * Save node and edge costs (or loss) under the given name.
	 */
	virtual void saveCosts(const Crag& crag, const Costs& costs, std::string name) = 0;

	/**
	 * Retrieve the candidate region adjacency graph (CRAG) associated to this 
	 * store.
	 */
	virtual void retrieveCrag(Crag& crag) = 0;

	/**
	 * Retrieve the volumes of CRAG candidates. For that, only the leaf node 
	 * volumes of the given CragVolumes are set, other volumes will later be 
	 * created on demand.
	 */
	virtual void retrieveVolumes(CragVolumes& volumes) = 0;

	/**
	 * Retrieve features for the candidates (i.e., the nodes) of the CRAG 
	 * associated to this store.
	 */
	virtual void retrieveNodeFeatures(const Crag& crag, NodeFeatures& features) = 0;

	/**
	 * Retrieve features for adjacent candidates (i.e., the edges) of the CRAG 
	 * associated to this store.
	 */
	virtual void retrieveEdgeFeatures(const Crag& crag, EdgeFeatures& features) = 0;

	/**
	 * Retrieve the min and max values of the node features.
	 */
	virtual void retrieveFeaturesMin(FeatureWeights& min) = 0;
	virtual void retrieveFeaturesMax(FeatureWeights& max) = 0;

	/**
	 * Retrieve skeletons for the candidates of the CRAG.
	 */
	virtual void retrieveSkeletons(const Crag& crag, Skeletons& skeletons) = 0;

	/**
	 * Retrieve the volume rays for the candidates of the CRAG.
	 */
	virtual void retrieveVolumeRays(VolumeRays& rays) = 0;

	/**
	 * Retrieve feature weights.
	 */
	virtual void retrieveFeatureWeights(FeatureWeights& weights) = 0;

	/**
	 * Retrieve node and edge costs (or loss) of the given name.
	 */
	virtual void retrieveCosts(const Crag& crag, Costs& costs, std::string name) = 0;

	/**
	 * Store a solution with a given name.
	 */
	virtual void saveSolution(
			const Crag&         crag,
			const CragSolution& solution,
			std::string         name) = 0;

	/**
	 * Retrieve the solution with the given name.
	 */
	virtual void retrieveSolution(
			const Crag&   crag,
			CragSolution& solution,
			std::string   name) = 0;

	/**
	 * Get a list of the names of all stored solutions.
	 */
	virtual std::vector<std::string> getSolutionNames() = 0;
};

#endif // TREE_MC_IO_CRAG_STORE_H__

