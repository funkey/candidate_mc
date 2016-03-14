#ifndef CANDIDATE_MC_LEARNING_LOSS_H__
#define CANDIDATE_MC_LEARNING_LOSS_H__

#include <set>
#include <inference/Costs.h>
#include <inference/MultiCutSolver.h>

/**
 * A loss function that factorizes into additive contributions of the CRAG nodes 
 * and edges, plus a constant value.
 */
class Loss : public Costs {

public:

	Loss(const Crag& crag) : Costs(crag) {}

	/**
	 * Normalize the node and edge values, such that the loss is always between 
	 * 0 and 1. For that, the loss is minimized and maximized on the given CRAG, 
	 * with the given multi-cut parameters.
	 */
	void normalize(const Crag& crag, const MultiCutSolver::Parameters& parameters);

	double constant;
};

#endif // CANDIDATE_MC_LEARNING_LOSS_H__

