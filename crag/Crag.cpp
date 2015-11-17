#include "Crag.h"
#include <util/assert.h>

Crag::CragNode Crag::Invalid;

const std::vector<Crag::NodeType> Crag::NodeTypes = { VolumeNode, SliceNode, AssignmentNode, NoAssignmentNode };
const std::vector<Crag::EdgeType> Crag::EdgeTypes = { AdjacencyEdge, AssignmentEdge, NoAssignmentEdge };

int
Crag::getLevel(Crag::CragNode n) const {

	if (isLeafNode(n))
		return 0;

	int level = 0;
	for (SubsetInArcIt e(getSubsetGraph(), toSubset(n)); e != lemon::INVALID; ++e)
		level = std::max(getLevel(toRag(getSubsetGraph().source(e))), level);

	return level + 1;
}

std::set<Crag::CragNode>
Crag::leafNodes(CragNode n) const {

	std::set<CragNode> leafNodes;
	recLeafNodes(n, leafNodes);

	return leafNodes;
}

std::set<Crag::CragEdge>
Crag::leafEdges(CragEdge e) const {

	std::set<CragEdge> leafEdges;
	std::set<CragNode> uLeafNodes;
	std::set<CragNode> vLeafNodes;

	recLeafNodes(u(e), uLeafNodes);
	recLeafNodes(v(e), vLeafNodes);

	for (CragNode n : uLeafNodes)
		for (CragEdge e : adjEdges(n))
			if (isLeafEdge(e))
				if (vLeafNodes.count(oppositeNode(n, e)))
					leafEdges.insert(e);

	return leafEdges;
}

void
Crag::recLeafNodes(CragNode n, std::set<CragNode>& leafNodes) const {

	if (isLeafNode(n))
		leafNodes.insert(n);
	else
		for (auto a : inArcs(n))
			recLeafNodes(a.source(), leafNodes);
}
