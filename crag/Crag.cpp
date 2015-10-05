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

