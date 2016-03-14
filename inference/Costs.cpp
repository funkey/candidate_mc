#include "Costs.h"

void
Costs::propagateLeafValues(const Crag& crag) {

	std::map<Crag::CragNode, std::set<Crag::CragNode>> leafNodes;

	// collect leaf nodes under each crag node
	for (Crag::CragNode n : crag.nodes())
		if (crag.isRootNode(n))
			collectLeafNodes(crag, n, leafNodes);

	// get node values
	for (Crag::CragNode n : crag.nodes())
		node[n] = nodeValueFromLeafNodes(crag, n, leafNodes);

	// get edge values
	for (Crag::CragEdge e : crag.edges())
		edge[e] = edgeValueFromLeafNodes(crag, e, leafNodes);
}

void
Costs::propagateLeafEdgeValues(const Crag& crag) {

	std::map<Crag::CragNode, std::set<Crag::CragNode>> leafNodes;

	// collect leaf nodes under each crag node
	for (Crag::CragNode n : crag.nodes())
		if (crag.isRootNode(n))
			collectLeafNodes(crag, n, leafNodes);

	// get edge values
	for (Crag::CragEdge e : crag.edges())
		edge[e] = edgeValueFromLeafNodes(crag, e, leafNodes);
}

void
Costs::collectLeafNodes(
		const Crag&                                         crag,
		Crag::CragNode                                      n,
		std::map<Crag::CragNode, std::set<Crag::CragNode>>& leafNodes) {

	unsigned int numChildren = 0;
	for (Crag::CragArc e : crag.inArcs(n)) {

		Crag::CragNode child = e.source();

		collectLeafNodes(crag, child, leafNodes);
		numChildren++;

		for (Crag::CragNode m : leafNodes[child])
			leafNodes[n].insert(m);
	}

	if (numChildren == 0)
		leafNodes[n].insert(n);
}

double
Costs::nodeValueFromLeafNodes(
		const Crag&                                               crag,
		const Crag::CragNode&                                     n,
		const std::map<Crag::CragNode, std::set<Crag::CragNode>>& leafNodes) {

	double value = 0;

	// value of a node is the value of all contained leaf nodes plus all inner
	// leaf edges

	for (Crag::CragNode leaf : leafNodes.at(n)) {

		value += node[leaf];

		// all edges with leaf as u
		for (Crag::CragEdge e : crag.adjEdges(leaf)) {

			// leaf not u?
			if (crag.u(e) != leaf)
				continue;

			Crag::CragNode neighbor = crag.v(e);

			// not a contained leaf node?
			if (!leafNodes.at(n).count(neighbor))
				continue;

			// e is an inside leaf edge
			value += edge[e];
		}
	}

	return value;
}

double
Costs::edgeValueFromLeafNodes(
		const Crag&                                               crag,
		const Crag::CragEdge&                                     e,
		const std::map<Crag::CragNode, std::set<Crag::CragNode>>& leafNodes) {

	double value = 0;

	// value of an edge (u,v) is sum of values of all leaf edges between contained
	// leaf nodes of u and v

	const std::set<Crag::CragNode>& uLeafNodes = leafNodes.at(crag.u(e));
	const std::set<Crag::CragNode>& vLeafNodes = leafNodes.at(crag.v(e));

	// for each leaf node of u
	for (Crag::CragNode leaf : uLeafNodes) {

		// for each adjacency edge
		for (Crag::CragEdge f : crag.adjEdges(leaf)) {

			Crag::CragNode neighbor = crag.oppositeNode(leaf, f);

			// not a v-contained leaf node?
			if (!vLeafNodes.count(neighbor))
				continue;

			// f is an edge between leaf nodes of u and v
			value += edge[f];
		}
	}

	return value;
}
