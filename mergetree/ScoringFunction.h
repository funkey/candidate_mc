#ifndef MULTI2CUT_MERGETREE_SCORING_FUNCTION_H__
#define MULTI2CUT_MERGETREE_SCORING_FUNCTION_H__

#include <vigra/multi_gridgraph.hxx>
#include <vigra/adjacency_list_graph.hxx>

/**
 * Default (empty) interface definition for scoring functions to be used with 
 * IterativeRegionMerging.
 */
class ScoringFunction {

public:

	typedef vigra::GridGraph<2>       GridGraphType;
	typedef vigra::AdjacencyListGraph RagType;

	/**
	 * Called to score an edge.
	 */
	float operator()(const RagType::Edge& edge, std::vector<GridGraphType::Edge>& gridEdges) {
		return 0;
	}

	/**
	 * Called to inform about a merge. Use this to incrementally update internal 
	 * statistics.
	 */
	void onMerge(const RagType::Edge& edge, const RagType::Node newRegion) {}
};

#endif // MULTI2CUT_MERGETREE_SCORING_FUNCTION_H__

