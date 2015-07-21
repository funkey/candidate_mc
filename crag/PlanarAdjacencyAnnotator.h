#ifndef CANDIDATE_MC_CRAG_PLANAR_ADJACENCY_ANNOTATOR_H__
#define CANDIDATE_MC_CRAG_PLANAR_ADJACENCY_ANNOTATOR_H__

#include "AdjacencyAnnotator.h"

/**
 * A CRAG adjacency annotator that extends a given CRAG with adjacency edges, if 
 * the respective candidates are touching.
 */
class PlanarAdjacencyAnnotator : public AdjacencyAnnotator {

public:

	/**
	 * The neighborhood to chose to determine whether two candidates touch.
	 */
	enum Neighborhood {

		// consider only direct 6 neighbors
		Direct,

		// consider all 26 neighbors
		Indirect
	};

	PlanarAdjacencyAnnotator(Neighborhood neighborhood) :
		_neighborhood(neighborhood) {}

	/**
	 * Annotate the leaf nodes of the given CRAG with adjacency edges. An edge 
	 * is introduced, if the corresponding volumes are adjacent according to the 
	 * specified neighborhood (direct or indirect).
	 */
	void annotate(Crag& crag, const CragVolumes& volumes) override;

private:

	Neighborhood _neighborhood;
};

#endif // CANDIDATE_MC_CRAG_PLANAR_ADJACENCY_ANNOTATOR_H__

