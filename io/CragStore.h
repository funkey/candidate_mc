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
};

#endif // TREE_MC_IO_CRAG_STORE_H__

