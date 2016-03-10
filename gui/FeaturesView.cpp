#include "FeaturesView.h"
#include <util/helpers.hpp>

void
SoltionView::onSignal(SetEdge& signal) {

	if (_crag.type(signal.getEdge()) == Crag::AssignmentEdge)
		return;

	if (_edgeFeatures[signal.getEdge()].size() > 0)
		std::cout << "features of current edge: " << _edgeFeatures[signal.getEdge()] << std::endl;
}

void
SoltionView::onSignal(SetCandidate& signal) {

	if (_crag.type(signal.getCandidate()) == Crag::NoAssignmentNode)
		return;

	if (_nodeFeatures[signal.getCandidate()].size() > 0)
		std::cout << "features of current node: " << _nodeFeatures[signal.getCandidate()] << std::endl;
}
