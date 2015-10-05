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

	_numAdded = 0;
	for (Crag::CragNode n : crag.nodes())
		if (crag.isRootNode(n))
			recurseAdjacencies(crag, n);

	if (optionPruneChildEdges)
		pruneChildEdges(crag);

	LOG_USER(adjacencyannotatorlog)
			<< "added " << _numAdded << " super node adjacency edges"
			<< std::endl;
}

std::set<Crag::CragNode>
AdjacencyAnnotator::recurseAdjacencies(Crag& crag, Crag::CragNode n) {

	LOG_ALL(adjacencyannotatorlog) << "recursing into node " << crag.id(n) << std::endl;

	// get all leaf subnodes
	std::set<Crag::CragNode> subnodes;
	for (Crag::CragArc a : crag.inArcs(n)) {

		std::set<Crag::CragNode> a_subnodes = recurseAdjacencies(crag, a.source());

		for (Crag::CragNode s : a_subnodes)
			subnodes.insert(s);
	}

	// for each leaf subnode adjacent to a non-subnode, add an adjacency edge to 
	// the non-subnode
	std::set<Crag::CragNode> neighbors;

	LOG_ALL(adjacencyannotatorlog) << "subnodes of " << crag.id(n) << " are:" << std::endl;
	for (Crag::CragNode s : subnodes) {

		LOG_ALL(adjacencyannotatorlog) << "\t" << crag.id(s) << std::endl;

		for (Crag::CragEdge e : crag.adjEdges(s)) {

			Crag::CragNode neighbor = e.opposite(s);

			// not a subnode
			if (!subnodes.count(neighbor))
				neighbors.insert(neighbor);
		}
	}

	for (Crag::CragNode neighbor : neighbors) {

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

	std::vector<Crag::CragEdge> siblingEdges;

	for (Crag::CragEdge e : crag.edges())
		if (isSiblingEdge(crag, e))
			siblingEdges.push_back(e);

	for (Crag::CragEdge e : siblingEdges)
		crag.erase(e);

	LOG_USER(adjacencyannotatorlog) << "pruned " << siblingEdges.size() << " child adjacency edges" << std::endl;
}

bool
AdjacencyAnnotator::isSiblingEdge(Crag& crag, Crag::CragEdge e) {

	if (crag.isRootNode(e.u()) || crag.isRootNode(e.v()))
		return false;

	Crag::CragNode parentU = (*crag.outArcs(e.u()).begin()).target();
	Crag::CragNode parentV = (*crag.outArcs(e.v()).begin()).target();

	return (parentU == parentV);
}
