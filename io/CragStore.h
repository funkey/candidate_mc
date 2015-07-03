#ifndef TREE_MC_IO_CRAG_STORE_H__
#define TREE_MC_IO_CRAG_STORE_H__

#include <crag/Crag.h>
#include <features/NodeFeatures.h>
#include <features/EdgeFeatures.h>

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
	 * Retrieve the candidate region adjacency graph (CRAG) associated to this 
	 * store.
	 */
	virtual void retrieveCrag(Crag& crag) = 0;

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

