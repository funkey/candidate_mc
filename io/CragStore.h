#ifndef TREE_MC_IO_CRAG_STORE_H__
#define TREE_MC_IO_CRAG_STORE_H__

#include <crag/Crag.h>

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
	 * Retrieve the candidate region adjacency graph (CRAG) associated to this 
	 * store.
	 */
	virtual void retrieveCrag(Crag& crag) = 0;
};

#endif // TREE_MC_IO_CRAG_STORE_H__

