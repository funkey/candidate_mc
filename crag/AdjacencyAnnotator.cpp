#include <util/Logger.h>
#include <util/helpers.hpp>
#include <util/ProgramOptions.h>
#include "AdjacencyAnnotator.h"

logger::LogChannel adjacencyannotatorlog("adjacencyannotatorlog", "[AdjacencyAnnotator] ");

util::ProgramOption optionPruneChildEdges(
		util::_long_name        = "pruneChildEdges",
		util::_description_text = "For binary trees, remove adjacency edges between children, since the "
		                          "merge represented by those is already performed by selecting the parent "
		                          "node.");

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

	if (optionPruneChildEdges)
		pruneChildEdges(crag);

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

	// for each leaf subnode adjacent to a non-subnode, add an adjacency edge to 
	// the non-subnode
	std::set<Crag::Node> neighbors;

	LOG_ALL(adjacencyannotatorlog) << "subnodes of " << crag.id(n) << " are:" << std::endl;
	for (Crag::Node s : subnodes) {

		LOG_ALL(adjacencyannotatorlog) << "\t" << crag.id(s) << std::endl;

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

	subnodes.insert(n);

	LOG_ALL(adjacencyannotatorlog) << "leaving node " << crag.id(n) << std::endl;

	return subnodes;
}

void
AdjacencyAnnotator::pruneChildEdges(Crag& crag) {

	std::vector<Crag::Edge> siblingEdges;

	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e)
		if (isSiblingEdge(crag, e))
			siblingEdges.push_back(e);

	for (Crag::Edge e : siblingEdges)
		crag.erase(e);

	LOG_USER(adjacencyannotatorlog) << "pruned " << siblingEdges.size() << " child adjacency edges" << std::endl;
}

bool
AdjacencyAnnotator::isSiblingEdge(Crag& crag, Crag::Edge e) {

	if (crag.isRootNode(crag.u(e)) || crag.isRootNode(crag.v(e)))
		return false;

	Crag::SubsetNode parentU = crag.getSubsetGraph().target(Crag::SubsetOutArcIt(crag, crag.toSubset(crag.u(e))));
	Crag::SubsetNode parentV = crag.getSubsetGraph().target(Crag::SubsetOutArcIt(crag, crag.toSubset(crag.v(e))));

	return (parentU == parentV);
}
