#include "Crag.h"
#include <util/assert.h>

Crag::CragNode Crag::Invalid;

const std::vector<Crag::NodeType> Crag::NodeTypes = { VolumeNode, SliceNode, AssignmentNode, NoAssignmentNode };
const std::vector<Crag::EdgeType> Crag::EdgeTypes = { AdjacencyEdge, SeparationEdge, AssignmentEdge, NoAssignmentEdge };

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
Crag::leafEdges(CragNode n) const {

	std::set<CragNode> nleafNodes = leafNodes(n);
	std::set<CragEdge> leafEdges;

	for (CragNode u : nleafNodes)
		for (CragNode v : nleafNodes) {

			if (id(u) >= id(v))
				continue;

			for (CragEdge e : adjEdges(u))
				if (isLeafEdge(e))
					if (nleafNodes.count(oppositeNode(u, e)))
						leafEdges.insert(e);
		}

	return leafEdges;
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

std::set<Crag::CragEdge>
Crag::descendantEdges(CragEdge e) const {

	std::set<CragEdge> descendants = descendantEdges(e.u(), e.v());
	descendants.erase(e);

	return descendants;
}

std::set<Crag::CragEdge>
Crag::descendantEdges(CragNode u, CragNode v) const {

	std::set<CragEdge> uEdges;
	std::set<CragEdge> vEdges;
	recCollectEdges(u, uEdges);
	recCollectEdges(v, vEdges);

	std::set<CragEdge> descendants;
	std::set_intersection(
		uEdges.begin(), uEdges.end(),
		vEdges.begin(), vEdges.end(),
		std::inserter(descendants, descendants.end()));

	return descendants;
}

void
Crag::recLeafNodes(CragNode n, std::set<CragNode>& leafNodes) const {

	if (isLeafNode(n))
		leafNodes.insert(n);
	else
		for (auto a : inArcs(n))
			recLeafNodes(a.source(), leafNodes);
}

void
Crag::recCollectEdges(Crag::CragNode n, std::set<Crag::CragEdge>& edges) const {

	for (Crag::CragEdge e : adjEdges(n))
		edges.insert(e);

	for (Crag::CragArc a : inArcs(n))
		recCollectEdges(a.source(), edges);
}
