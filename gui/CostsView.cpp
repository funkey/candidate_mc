#include "CostsView.h"

void
CostsView::onSignal(SetCandidate& signal) {

	std::cout << _name << " of current candidate: " << _costs.node[signal.getCandidate()] << std::endl;
}

void
CostsView::onSignal(SetEdge& signal) {

	std::cout << _name << " of current edge: " << _costs.edge[signal.getEdge()] << std::endl;
}

