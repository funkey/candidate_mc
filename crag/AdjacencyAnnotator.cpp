#include <util/Logger.h>
#include <util/helpers.hpp>
#include "AdjacencyAnnotator.h"

logger::LogChannel adjacencyannotatorlog("adjacencyannotatorlog", "[AdjacencyAnnotator] ");

void
AdjacencyAnnotator::propagateLeafAdjacencies(Crag& crag) {

	// find root nodes
	std::vector<Crag::Node> roots;

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		int numParents = 0;
		for (Crag::SubsetOutArcIt a(crag.getSubsetGraph(), crag.toSubset(n)); a != lemon::INVALID; ++a)
			numParents++;

		if (numParents == 0)
			roots.push_back(n);
	}

	_numAdded = 0;
	for (Crag::Node n : roots)
		recurseAdjacencies(crag, n);

	LOG_USER(adjacencyannotatorlog)
			<< "added " << _numAdded << " super node adjacency edges"
			<< std::endl;
}

std::set<Crag::Node>
AdjacencyAnnotator::recurseAdjacencies(Crag& crag, Crag::Node n) {

	LOG_ALL(adjacencyannotatorlog) << "recursing into node " << crag.id(n) << std::endl;

	// get all leaf subnodes
	std::set<Crag::Node> subnodes;
	for (Crag::SubsetInArcIt a(crag.getSubsetGraph(), crag.toSubset(n)); a != lemon::INVALID; ++a) {

		std::set<Crag::Node> a_subnodes = recurseAdjacencies(crag, crag.toRag(crag.getSubsetGraph().source(a)));

		for (Crag::Node s : a_subnodes)
			subnodes.insert(s);
	}

	// for each leaf subnode adjacent to a non-subnode, add an adjacency edge 
	// to the non-subnode
	std::set<Crag::Node> neighbors;
	for (Crag::Node s : subnodes) {

		for (Crag::IncEdgeIt e(crag, s); e != lemon::INVALID; ++e) {

			Crag::Node neighbor = crag.getAdjacencyGraph().oppositeNode(s, e);

			// not a subnode
			if (!subnodes.count(neighbor))
				neighbors.insert(neighbor);
		}
	}

	for (Crag::Node neighbor : neighbors) {

		LOG_ALL(adjacencyannotatorlog)
				<< "adding propagated edge between "
				<< crag.id(n) << " and " << crag.id(neighbor)
				<< std::endl;

		crag.addAdjacencyEdge(n, neighbor);
	}

	_numAdded += neighbors.size();

	// only leaf nodes
	if (subnodes.empty())
		subnodes.insert(n);

	return subnodes;
}
