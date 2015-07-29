#ifndef CANDIDATE_MC_LEARNING_HAMMING_LOSS_H__
#define CANDIDATE_MC_LEARNING_HAMMING_LOSS_H__

#include "Loss.h"
#include "BestEffort.h"

class HammingLoss : public Loss {

public:

	/**
	 * Create a new Hamming loss from a given best-effort solution.
	 *
	 * @param crag
	 *              The CRAG the best-effort solution was computed for.
	 * @param bestEffort
	 *              The best-effort solution.
	 * @param balance
	 *              If 1, the loss will be balanced, such that two solutions 
	 *              that generate the same segmentation have the same loss. If 0 
	 *              the loss will not be balanced. If 2 (default), the program 
	 *              option optionBalanceHammingLoss will be consulted.
	 */
	HammingLoss(const Crag& crag, const BestEffort& bestEffort, int balance = 2);

private:

	bool isBestEffort(Crag::CragNode n, const Crag& crag, const BestEffort& bestEffort);
	bool isBestEffort(Crag::CragEdge e, const Crag& crag, const BestEffort& bestEffort);

	std::vector<Crag::CragNode> getPath(Crag::CragNode n, const Crag& crag);

	// balance the loss, such that two solutions with same segmentation have 
	// same loss
	bool _balance;
};

#endif // CANDIDATE_MC_LEARNING_HAMMING_LOSS_H__

