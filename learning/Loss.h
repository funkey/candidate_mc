#ifndef CANDIDATE_MC_LEARNING_LOSS_H__
#define CANDIDATE_MC_LEARNING_LOSS_H__

#include <crag/Crag.h>

/**
 * A loss function that factorizes into additive contributions of the CRAG nodes 
 * and edges, plus a constant value.
 */
struct Loss {

	Loss(const Crag& crag) : node(crag), edge(crag) {}

	Crag::NodeMap<double> node;
	Crag::EdgeMap<double> edge;

	double constant;
};

#endif // CANDIDATE_MC_LEARNING_LOSS_H__

