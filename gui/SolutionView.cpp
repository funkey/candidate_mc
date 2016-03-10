#include "SolutionView.h"

void
SolutionView::onSignal(SetEdge& signal) {

	std::cout << "current edge is part of solution \"" << _solutionName
			  << ": " << _solution.selected(signal.getEdge())
			  << std::endl;
}

void
SolutionView::onSignal(SetCandidate& signal) {

	std::cout << "current node is part of solution \"" << _solutionName
			  << ": " << _solution.selected(signal.getCandidate())
			  << std::endl;
}

