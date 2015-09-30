#include "TopologicalLoss.h"
#include <util/Logger.h>
#include <util/ProgramOptions.h>

logger::LogChannel topologicallosslog("topologicallosslog", "[TopologicalLoss] ");

util::ProgramOption optionTopologicalLossWeightSplit(
		util::_module           = "loss.topological",
		util::_long_name        = "weightSplit",
		util::_description_text = "The weight of a split error in the topological loss. Default is 1.0.",
		util::_default_value    = 1.0);

util::ProgramOption optionTopologicalLossWeightMerge(
		util::_module           = "loss.topological",
		util::_long_name        = "weightMerge",
		util::_description_text = "The weight of a merge error in the topological loss. Default is 1.0.",
		util::_default_value    = 1.0);

util::ProgramOption optionTopologicalLossWeightFp(
		util::_module           = "loss.topological",
		util::_long_name        = "weightFp",
		util::_description_text = "The weight of a false positive error in the topological loss. Default is 1.0.",
		util::_default_value    = 1.0);

util::ProgramOption optionTopologicalLossWeightFn(
		util::_module           = "loss.topological",
		util::_long_name        = "weightFn",
		util::_description_text = "The weight of a false negative error in the topological loss. Default is 1.0.",
		util::_default_value    = 1.0);

TopologicalLoss::TopologicalLoss(
		const Crag&       crag,
		const BestEffort& bestEffort) :
	Loss(crag),
	_weightSplit(optionTopologicalLossWeightSplit),
	_weightMerge(optionTopologicalLossWeightMerge),
	_weightFp(optionTopologicalLossWeightFp),
	_weightFn(optionTopologicalLossWeightFn) {

	for (Crag::CragEdge e : crag.edges())
		edge[e] = 0;

	constant = 0;

	// for each candidate tree
	for (Crag::CragNode n : crag.nodes())
		if (crag.isRootNode(n))
			traverseAboveBestEffort(crag, n, bestEffort);
}

TopologicalLoss::NodeCosts
TopologicalLoss::traverseAboveBestEffort(const Crag& crag, Crag::CragNode n, const BestEffort& bestEffort) {

	bool isBestEffort = bestEffort.selected(n);

	LOG_DEBUG(topologicallosslog) << "entering node " << crag.id(n) << std::endl;
	LOG_DEBUG(topologicallosslog) << "\tis best effort: " << isBestEffort << std::endl;

	if (isBestEffort) {

		NodeCosts bestEffortCosts;
		bestEffortCosts.split = 0;
		bestEffortCosts.merge = 0;
		bestEffortCosts.fp    = 0;
		bestEffortCosts.fn    = -_weightFn;
		constant += _weightFn;

		// this assigns the cost to node and all descendants
		traverseBelowBestEffort(crag, n, bestEffortCosts);

		return bestEffortCosts;
	}

	unsigned int numChildren = crag.inArcs(n).size();

	LOG_DEBUG(topologicallosslog) << "\tthis slice has " << numChildren << " children" << std::endl;

	if (numChildren == 0) {

		// We are above best-effort, and we don't have children -- this slice 
		// belongs to a path that is completely spurious.

		// give it false positive costs
		NodeCosts falsePositiveCosts;
		falsePositiveCosts.split = 0;
		falsePositiveCosts.merge = 0;
		falsePositiveCosts.fp    = _weightFp;
		falsePositiveCosts.fn    = 0;

		node[n] = falsePositiveCosts;

		return falsePositiveCosts;
	}

	// we are above the best effort solution

	// get our node costs from the costs of our children
	double sumChildMergeCosts = 0;
	double sumChildFnCosts    = 0;
	double minChildFpCosts    = std::numeric_limits<double>::infinity();
	for (Crag::CragArc arc : crag.inArcs(n)) {

		NodeCosts childCosts = traverseAboveBestEffort(crag, arc.source(), bestEffort);

		sumChildMergeCosts += childCosts.merge;
		sumChildFnCosts    += childCosts.fn;
		minChildFpCosts     = std::min(minChildFpCosts, childCosts.fp);
	}

	NodeCosts costs;
	costs.split = 0;
	costs.merge = _weightMerge*(numChildren - 1) + sumChildMergeCosts;
	costs.fn    = sumChildFnCosts;
	costs.fp    = minChildFpCosts;

	LOG_DEBUG(topologicallosslog) << "\tthis slice is above best-effort, assign total costs of " << costs << std::endl;

	node[n] = costs;

	return costs;
}

void
TopologicalLoss::traverseBelowBestEffort(const Crag& crag, Crag::CragNode n, NodeCosts costs) {

	LOG_DEBUG(topologicallosslog) << "entering node " << crag.id(n) << std::endl;

	node[n] = costs;

	// number of children
	double k = crag.inArcs(n).size();

	// get the children's costs
	NodeCosts childCosts;
	childCosts.split = costs.split + _weightSplit*(k - 1)/k;
	childCosts.merge = 0;
	childCosts.fn    = costs.fn/k;
	childCosts.fp    = 0;

	// propagate costs downwards
	for (Crag::CragArc arc : crag.inArcs(n))
		traverseBelowBestEffort(crag, arc.source(), childCosts);
}
