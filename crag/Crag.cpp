#include "Crag.h"
#include <util/assert.h>

int
Crag::getLevel(Crag::Node n) const {

	if (isLeafNode(n))
		return 0;

	int level = 0;
	for (SubsetInArcIt e(getSubsetGraph(), toSubset(n)); e != lemon::INVALID; ++e)
		level = std::max(getLevel(toRag(getSubsetGraph().source(e))), level);

	return level + 1;
}

