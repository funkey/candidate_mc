#ifndef CANDIDATE_MC_GUI_COSTS_VIEW_H__
#define CANDIDATE_MC_GUI_COSTS_VIEW_H__

#include <crag/Crag.h>
#include <inference/Costs.h>
#include <scopegraph/Agent.h>
#include "Signals.h"

class CostsView :
		public sg::Agent<
				CostsView,
				sg::Accepts<
						SetCandidate,
						SetEdge
				>
		> {

public:

	CostsView(
			const Crag&  crag,
			const Costs& costs,
			std::string name) :
		_crag(crag),
		_costs(costs),
		_name(name) {}

	void onSignal(SetCandidate& signal);

	void onSignal(SetEdge& signal);

private:

	const Crag&  _crag;
	const Costs& _costs;

	std::string _name;
};

#endif // CANDIDATE_MC_GUI_COSTS_VIEW_H__

