#ifndef CANDIDATE_MC_INFERENCE_COSTS_H__
#define CANDIDATE_MC_INFERENCE_COSTS_H__

#include <crag/Crag.h>

struct Costs {

	Costs(const Crag& crag) : node(crag), edge(crag) {}

	Crag::NodeMap<double> node;
	Crag::EdgeMap<double> edge;
};

#endif // CANDIDATE_MC_INFERENCE_COSTS_H__

