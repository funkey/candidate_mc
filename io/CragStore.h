#ifndef TREE_MC_IO_CRAG_STORE_H__
#define TREE_MC_IO_CRAG_STORE_H__

#include <set>
#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <features/NodeFeatures.h>
#include <features/EdgeFeatures.h>
#include <features/Skeletons.h>

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
	virtual void saveNodeFeaturesMinMax(
			const std::vector<double>& min,
			const std::vector<double>& max) = 0;

	/**
	 * Store the min and max values of the edge features.
	 */
	virtual void saveEdgeFeaturesMinMax(
			const std::vector<double>& min,
			const std::vector<double>& max) = 0;

	/**
	 * Store the skeletons for candidates of a CRAG.
	 */
	virtual void saveSkeletons(const Crag& crag, const Skeletons& skeletons) = 0;

	/**
	 * Store a set of feature weights.
	 */
	virtual void saveFeatureWeights(const FeatureWeights& weights) = 0;

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
	virtual void retrieveNodeFeaturesMinMax(
			std::vector<double>& min,
			std::vector<double>& max) = 0;

	/**
	 * Retrieve the min and max values of the edge features.
	 */
	virtual void retrieveEdgeFeaturesMinMax(
			std::vector<double>& min,
			std::vector<double>& max) = 0;

	/**
	 * Retrieve skeletons for the candidates of the CRAG.
	 */
	virtual void retrieveSkeletons(const Crag& crag, Skeletons& skeletons) = 0;

	/**
	 * Retrieve feature weights.
	 */
	virtual void retrieveFeatureWeights(FeatureWeights& weights) = 0;

	/**
	 * Store a segmentation, represented by sets of leaf nodes.
	 */
	virtual void saveSegmentation(
			const Crag&                              crag,
			const std::vector<std::set<Crag::Node>>& segmentation,
			std::string                              name) = 0;

	/**
	 * Retrieve a segmentation, represented by sets of leaf nodes.
	 */
	virtual void retrieveSegmentation(
			const Crag&                        crag,
			std::vector<std::set<Crag::Node>>& segmentation,
			std::string                        name) = 0;

	/**
	 * Get a list of the names of all stored segmentations.
	 */
	virtual std::vector<std::string> getSegmentationNames() = 0;
};

#endif // TREE_MC_IO_CRAG_STORE_H__

