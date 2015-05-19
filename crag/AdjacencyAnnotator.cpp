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

	for (Crag::Node n : roots)
		recurseAdjacencies(crag, n);
}

std::set<Crag::Node>
AdjacencyAnnotator::recurseAdjacencies(Crag& crag, Crag::Node n) {

	LOG_ALL(adjacencyannotatorlog) << "recursing into node " << crag.id(n) << std::endl;

	// get all subnodes
	std::set<Crag::Node> subnodes;
	for (Crag::SubsetInArcIt a(crag.getSubsetGraph(), crag.toSubset(n)); a != lemon::INVALID; ++a) {

		std::set<Crag::Node> a_subnodes = recurseAdjacencies(crag, crag.toRag(crag.getSubsetGraph().source(a)));

		for (Crag::Node s : a_subnodes)
			subnodes.insert(s);
	}

	LOG_ALL(adjacencyannotatorlog) << "subnodes of " << crag.id(n) << " are: ";
	for (Crag::Node s : subnodes)
		LOG_ALL(adjacencyannotatorlog) << crag.id(s) << " ";
	LOG_ALL(adjacencyannotatorlog) << std::endl;

	// for each subnode adjacent to a non-subnode, add an adjacency edge
	for (Crag::Node s : subnodes) {

		for (Crag::IncEdgeIt e(crag, s); e != lemon::INVALID; ++e) {

			Crag::Node neighbor = (
					crag.getAdjacencyGraph().u(e) == s ?
					crag.getAdjacencyGraph().v(e) :
					crag.getAdjacencyGraph().u(e));

			// not a subnode
			if (!subnodes.count(neighbor)) {

				LOG_ALL(adjacencyannotatorlog)
						<< "adding propagated edge between "
						<< crag.id(n) << " and " << crag.id(neighbor)
						<< std::endl;

				crag.addAdjacencyEdge(n, neighbor);
			}
		}
	}

	subnodes.insert(n);
	return subnodes;
}
