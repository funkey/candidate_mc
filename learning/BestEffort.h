#ifndef CANDIDATE_MC_LEARNING_BEST_EFFORT_H__
#define CANDIDATE_MC_LEARNING_BEST_EFFORT_H__

#include <crag/Crag.h>

struct BestEffort {

	BestEffort(const Crag& crag) : node(crag), edge(crag) {}

	Crag::NodeMap<bool> node;
	Crag::EdgeMap<bool> edge;
};

#endif // CANDIDATE_MC_LEARNING_BEST_EFFORT_H__

