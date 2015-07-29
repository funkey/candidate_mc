#include "HammingLoss.h"
#include <util/ProgramOptions.h>
#include <util/assert.h>

util::ProgramOption optionBalanceHammingLoss(
		util::_module           = "loss.hamming",
		util::_long_name        = "balance",
		util::_description_text = "Assume that the best-effort is only defined on the leaf nodes "
		                          "and edges, and propagate the loss upwards, such that a solution "
		                          "that creates the same segmentation has the same loss.");

HammingLoss::HammingLoss(
		const Crag& crag,
		const BestEffort& bestEffort) :
	Loss(crag) {

	bool balance = optionBalanceHammingLoss;

	constant = 0;

	for (Crag::CragNode n : crag.nodes())
		if (bestEffort.node[n]) {

			if (balance)
				UTIL_ASSERT(crag.isLeafNode(n));;

			node[n] = -1;
			constant++;

		} else {

			if (!balance || crag.isLeafNode(n))
				node[n] = 1;
		}

	for (Crag::CragEdge e : crag.edges())
		if (bestEffort.edge[e]) {

			if (balance)
				UTIL_ASSERT(crag.isLeafEdge(e));;

			edge[e] = -1;
			constant++;

		} else {

			if (!balance || crag.isLeafEdge(e))
				edge[e] = 1;
		}

	if (balance)
		propagateLeafLoss(crag);
}
