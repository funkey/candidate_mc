#include "Crag.h"
#include <util/assert.h>

int
Crag::getLevel(Crag::Node n) const {

	if (SubsetInArcIt(*this, toSubset(n)) == lemon::INVALID)
		return 0;

	int level = 0;
	for (SubsetInArcIt e(*this, toSubset(n)); e != lemon::INVALID; ++e) {

		level = std::max(getLevel(toRag(getSubsetGraph().oppositeNode(toSubset(n), e))), level);
	}

	return level + 1;
}

