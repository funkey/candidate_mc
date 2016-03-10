#ifndef CANDIDATE_MC_GUI_FEATURES_VIEW_H__
#define CANDIDATE_MC_GUI_FEATURES_VIEW_H__

#include <crag/Crag.h>
#include <features/NodeFeatures.h>
#include <features/EdgeFeatures.h>
#include <scopegraph/Agent.h>
#include "Signals.h"

class SoltionView :
		public sg::Agent<
				SoltionView,
				sg::Accepts<
						SetCandidate,
						SetEdge
				>
		> {

public:

	SoltionView(
			const Crag&         crag,
			const NodeFeatures& nodeFeatures,
			const EdgeFeatures& edgeFeatures) :
		_crag(crag),
		_nodeFeatures(nodeFeatures),
		_edgeFeatures(edgeFeatures) {}

	void onSignal(SetCandidate& signal);

	void onSignal(SetEdge& signal);

private:

	const Crag&         _crag;
	const NodeFeatures& _nodeFeatures;
	const EdgeFeatures& _edgeFeatures;
};

#endif // CANDIDATE_MC_GUI_FEATURES_VIEW_H__

