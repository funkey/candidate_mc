#ifndef CANDIDATE_MC_GUI_COSTS_VIEW_H__
#define CANDIDATE_MC_GUI_COSTS_VIEW_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <inference/Costs.h>
#include <scopegraph/Agent.h>
#include <sg_gui/KeySignals.h>
#include "Signals.h"

class CostsView :
		public sg::Agent<
				CostsView,
				sg::Provides<
						sg_gui::ContentChanged
				>,
				sg::Accepts<
						sg_gui::Draw,
						sg_gui::KeyDown,
						SetCandidate,
						SetEdge
				>
		> {

public:

	CostsView(
			const Crag&  crag,
			const CragVolumes& volumes,
			const Costs& costs,
			std::string name);

	void onSignal(sg_gui::Draw& signal);

	void onSignal(sg_gui::KeyDown& signal);

	void onSignal(SetCandidate& signal);

	void onSignal(SetEdge& signal);

private:

	const Crag& _crag;
	const CragVolumes& _volumes;
	const Costs& _costs;

	std::string _name;

	bool _showNeighborLinks;
	Crag::CragNode _currentNode;
	float _costsScale;
};

#endif // CANDIDATE_MC_GUI_COSTS_VIEW_H__

