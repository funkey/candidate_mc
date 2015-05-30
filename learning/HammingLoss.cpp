#include "HammingLoss.h"

HammingLoss::HammingLoss(
		const Crag& crag,
		const BestEffort& bestEffort) :
	Loss(crag) {

	constant = 0;

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		if (bestEffort.node[n]) {

			node[n] = -1;
			constant++;

		} else {

			node[n] = 1;
		}

	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e)
		if (bestEffort.edge[e]) {

			edge[e] = -1;
			constant++;

		} else {

			edge[e] = 1;
		}
}
