#ifndef CANDIDATE_MC_LEARNING_BEST_EFFORT_H__
#define CANDIDATE_MC_LEARNING_BEST_EFFORT_H__

#include <crag/Crag.h>
#include <inference/MultiCut.h>

struct BestEffort {

	/**
	 * Crate a new uninitialized best effort.
	 */
	BestEffort(const Crag& crag) : node(crag), edge(crag) {}

	/**
	 * Create a best-effort by solving the multicut problem with the given 
	 * costs.
	 */
	BestEffort(
			const Crag&                 crag,
			const Costs&                costs,
			const MultiCut::Parameters& params = MultiCut::Parameters());

	Crag::NodeMap<bool> node;
	Crag::EdgeMap<bool> edge;
};

#endif // CANDIDATE_MC_LEARNING_BEST_EFFORT_H__

