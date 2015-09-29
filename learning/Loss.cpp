#include "Loss.h"
#include <util/Logger.h>

logger::LogChannel losslog("losslog", "[Loss] ");

void
Loss::propagateLeafLoss(const Crag& crag) {

	// find root nodes
	std::vector<Crag::Node> roots;
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		if (Crag::SubsetOutArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID)
			roots.push_back(n);

	// collect leaf nodes under each crag node
	std::map<Crag::Node, std::set<Crag::Node>> leafNodes;
	for (Crag::Node n : roots)
		collectLeafNodes(crag, n, leafNodes);

	// get node losses
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		node[n] = nodeLossFromLeafNodes(crag, n, leafNodes);

	// get edge losses
	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e)
		edge[e] = edgeLossFromLeafNodes(crag, e, leafNodes);
}

void
Loss::normalize(const Crag& crag, const MultiCutSolver::Parameters& params) {

	double min, max;

	{
		LOG_DEBUG(losslog) << "searching for minimal loss value..." << std::endl;

		MultiCutSolver::Parameters minParams(params);
		minParams.minimize = true;
		MultiCutSolver multicut(crag, minParams);
		multicut.setCosts(*this);
		multicut.solve();
		min = multicut.getValue();

		LOG_DEBUG(losslog) << "minimal value is " << min << std::endl;
	}
	{
		LOG_DEBUG(losslog) << "searching for maximal loss value..." << std::endl;

		MultiCutSolver::Parameters maxParams(params);
		maxParams.minimize = false;
		MultiCutSolver multicut(crag, maxParams);
		multicut.setCosts(*this);
		multicut.solve();
		max = multicut.getValue();

		LOG_DEBUG(losslog) << "maximal value is " << max << std::endl;
	}

	// All energies are between E(y^min) and E(y^max). We want E(y^min) to be 
	// zero, and E(y^max) to be 1. Therefore, we subtract min from each E(y) and 
	// scale it with 1.0/(max - min):
	//
	//  (E(y) + offset)*scale
	//
	//           = ([sum_i E(y_i)] + offset)*scale
	//           = ([sum_i E(y_i)]*scale + offset*scale)
	//
	// -> We scale each loss by scale, and add offset*scale to the constant.

	double offset = -min;
	double scale  = 1.0/(max - min);

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		node[n] *= scale;
	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e)
		edge[e] *= scale;

	constant += offset*scale;
}

void
Loss::collectLeafNodes(
		const Crag&                                 crag,
		Crag::Node                                  n,
		std::map<Crag::Node, std::set<Crag::Node>>& leafNodes) {

	unsigned int numChildren = 0;
	for (Crag::SubsetInArcIt e(crag.getSubsetGraph(), crag.toSubset(n)); e != lemon::INVALID; ++e) {

		Crag::Node child = crag.toRag(crag.getSubsetGraph().source(e));

		collectLeafNodes(crag, child, leafNodes);
		numChildren++;

		for (Crag::Node m : leafNodes[child])
			leafNodes[n].insert(m);
	}

	if (numChildren == 0)
		leafNodes[n].insert(n);
}

double
Loss::nodeLossFromLeafNodes(
		const Crag&                                       crag,
		const Crag::Node&                                 n,
		const std::map<Crag::Node, std::set<Crag::Node>>& leafNodes) {

	double loss = 0;

	// loss of a node is the loss of all contained leaf nodes plus all inner 
	// leaf edges

	for (Crag::Node leaf : leafNodes.at(n)) {

		loss += node[leaf];

		// all edges with leaf as u
		for (Crag::IncEdgeIt e(crag, leaf); e != lemon::INVALID; ++e) {

			// leaf not u?
			if (crag.u(e) != leaf)
				continue;

			Crag::Node neighbor = crag.v(e);

			// not a contained leaf node?
			if (!leafNodes.at(n).count(neighbor))
				continue;

			// e is an inside leaf edge
			loss += edge[e];
		}
	}

	return loss;
}

double
Loss::edgeLossFromLeafNodes(
		const Crag&                                       crag,
		const Crag::Edge&                                 e,
		const std::map<Crag::Node, std::set<Crag::Node>>& leafNodes) {

	double loss = 0;

	// loss of an edge (u,v) is sum of loss of all leaf edges between contained 
	// leaf nodes of u and v

	const std::set<Crag::Node>& uLeafNodes = leafNodes.at(crag.u(e));
	const std::set<Crag::Node>& vLeafNodes = leafNodes.at(crag.v(e));

	// for each leaf node of u
	for (Crag::Node leaf : uLeafNodes) {

		// for each adjacency edge
		for (Crag::IncEdgeIt f(crag, leaf); f != lemon::INVALID; ++f) {

			Crag::Node neighbor = crag.getAdjacencyGraph().oppositeNode(leaf, f);

			// not a v-contained leaf node?
			if (!vLeafNodes.count(neighbor))
				continue;

			// f is an edge between leaf nodes of u and v
			loss += edge[f];
		}
	}

	return loss;
}

