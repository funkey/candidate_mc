#include "HammingLoss.h"
#include <util/ProgramOptions.h>
#include <util/assert.h>

util::ProgramOption optionBalanceHammingLoss(
		util::_module           = "loss.hamming",
		util::_long_name        = "balance",
		util::_description_text = "Project the best-effort on the leaf nodes and edges, compute the loss there, "
		                          "and propagate the loss upwards, such that a solution that creates the same "
		                          "segmentation has the same loss.");

HammingLoss::HammingLoss(
		const Crag&       crag,
		const BestEffort& bestEffort,
		int               balance) :
	Loss(crag),
	_balance((balance == 2 && optionBalanceHammingLoss) || (balance == 1)) {

	constant = 0;

	for (Crag::CragNode n : crag.nodes())
		if (isBestEffort(n, crag, bestEffort)) {

			if (_balance)
				UTIL_ASSERT(crag.isLeafNode(n));;

			node[n] = -1;
			constant++;

		} else {

			if (!_balance || crag.isLeafNode(n))
				node[n] = 1;
		}

	for (Crag::CragEdge e : crag.edges())
		if (isBestEffort(e, crag, bestEffort)) {

			if (_balance)
				UTIL_ASSERT(crag.isLeafEdge(e));;

			edge[e] = -1;
			constant++;

		} else {

			if (!_balance || crag.isLeafEdge(e))
				edge[e] = 1;
		}

	if (_balance)
		propagateLeafLoss(crag);
}

bool
HammingLoss::isBestEffort(Crag::CragNode n, const Crag& crag, const BestEffort& bestEffort) {

	if (!_balance)
		return bestEffort.node[n];

	// if balanced, non-leaf nodes are not considered part of best-effort
	if (!crag.isLeafNode(n))
		return false;

	// is any of the parents best-effort?
	while (true) {

		if (bestEffort.node[n])
			return true;

		if (crag.isRootNode(n))
			return false;

		n = (*crag.outArcs(n).begin()).target();
	}
}

bool
HammingLoss::isBestEffort(Crag::CragEdge e, const Crag& crag, const BestEffort& bestEffort) {

	if (!_balance)
		return bestEffort.edge[e];

	// if balanced, non-leaf edges are not considered part of best-effort
	if (!crag.isLeafEdge(e))
		return false;

	std::vector<Crag::CragNode> uPath = getPath(e.u(), crag);
	std::vector<Crag::CragNode> vPath = getPath(e.v(), crag);

	// are the paths of u and v merging somewhere?
	for (Crag::CragNode u : uPath)
		for (Crag::CragNode v : vPath) {

			if (u == v && bestEffort.node[u])
				return true;

			for (Crag::CragEdge e : crag.adjEdges(u))
				if (crag.oppositeNode(u, e) == v && bestEffort.edge[e])
					return true;
		}

	return false;
}

std::vector<Crag::CragNode>
HammingLoss::getPath(Crag::CragNode n, const Crag& crag) {

	std::vector<Crag::CragNode> path;

	while (true) {

		path.push_back(n);

		if (crag.isRootNode(n))
			break;

		n = (*crag.outArcs(n).begin()).target();
	}

	return path;
}
