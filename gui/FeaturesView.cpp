#include "FeaturesView.h"
#include <util/helpers.hpp>

void
FeaturesView::onSignal(SetCandidate& /*signal*/) {

}

void
FeaturesView::onSignal(SetEdge& signal) {

	if (_crag.type(signal.getEdge()) == Crag::AssignmentEdge)
		return;

	if (_edgeFeatures[signal.getEdge()].size() > 0)
		std::cout << "features of current edge: " << _edgeFeatures[signal.getEdge()] << std::endl;
}
