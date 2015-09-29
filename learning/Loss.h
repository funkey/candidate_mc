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
	 * Propagate loss values of the leaf nodes and edges upwards, such that 
	 * different solutions resulting in the same segmentation have the same 
	 * loss. This function assumes that the leaf node and leaf edge loss values 
	 * have been set. A leaf edge is an edge between two leaf nodes.
	 */
	void propagateLeafLoss(const Crag& crag);

	/**
	 * Normalize the node and edge values, such that the loss is always between 
	 * 0 and 1. For that, the loss is minimized and maximized on the given CRAG, 
	 * with the given multi-cut parameters.
	 */
	void normalize(const Crag& crag, const MultiCutSolver::Parameters& parameters);

	double constant;

private:

	void collectLeafNodes(
			const Crag&                                 crag,
			Crag::Node                                  n,
			std::map<Crag::Node, std::set<Crag::Node>>& leafNodes);

	double nodeLossFromLeafNodes(
			const Crag&                                       crag,
			const Crag::Node&                                 n,
			const std::map<Crag::Node, std::set<Crag::Node>>& leafNodes);

	double edgeLossFromLeafNodes(
			const Crag&                                       crag,
			const Crag::Edge&                                 e,
			const std::map<Crag::Node, std::set<Crag::Node>>& leafNodes);
};

#endif // CANDIDATE_MC_LEARNING_LOSS_H__

