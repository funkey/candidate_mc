#ifndef CANDIDATE_MC_CRAG_ADJACENCY_ANNOTATOR_H__
#define CANDIDATE_MC_CRAG_ADJACENCY_ANNOTATOR_H__

#include "Crag.h"

/**
 * Base class for adjacency annotators.
 */
class AdjacencyAnnotator {

public:

	virtual void annotate(Crag& crag) = 0;

protected:

	/**
	 * Propagate adjacency of leaf candidates in a straight forward manner to 
	 * super-candidates: Candidates are adjacent, if any of their sub-candidates 
	 * are adjacent.
	 */
	void propagateLeafAdjacencies(Crag& crag);

private:

	std::set<Crag::Node> recurseAdjacencies(Crag& crag, Crag::Node n);

	unsigned int _numAdded;
};

#endif // CANDIDATE_MC_CRAG_ADJACENCY_ANNOTATOR_H__

