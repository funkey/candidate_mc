#ifndef CANDIDATE_MC_CRAG_ADJACENCY_ANNOTATOR_H__
#define CANDIDATE_MC_CRAG_ADJACENCY_ANNOTATOR_H__

#include "Crag.h"
#include "CragVolumes.h"

/**
 * Base class for adjacency annotators.
 */
class AdjacencyAnnotator {

public:

	virtual void annotate(Crag& crag, const CragVolumes& volumes) = 0;

protected:

	/**
	 * Propagate adjacency of leaf candidates in a straight forward manner to 
	 * super-candidates: Candidates are adjacent, if any of their sub-candidates 
	 * are adjacent.
	 */
	void propagateLeafAdjacencies(Crag& crag);

private:

	/**
	 * Find propagated edges for node n and below. Returns the set of 
	 * descendants of n (including n).
	 */
	std::set<Crag::Node> recurseAdjacencies(Crag& crag, Crag::Node n);

	/**
	 * For binary trees, remove adjacency edges between children, since the 
	 * merge represented by those is already performed by selecting the parent 
	 * node.
	 */
	void pruneChildEdges(Crag& crag);

	/**
	 * Is the given edge connecting children of the same node?
	 */
	bool isSiblingEdge(Crag& crag, Crag::Edge e);

	unsigned int _numAdded;
};

#endif // CANDIDATE_MC_CRAG_ADJACENCY_ANNOTATOR_H__

