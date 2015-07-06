#include "FeaturesView.h"
#include <util/helpers.hpp>

void
FeaturesView::onSignal(SetCandidate& signal) {

}

void
FeaturesView::onSignal(SetEdge& signal) {

	std::cout << _edgeFeatures[signal.getEdge()] << std::endl;
}
