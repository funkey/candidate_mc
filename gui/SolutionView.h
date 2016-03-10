#ifndef CANDIDATE_MC_GUI_SOLUTION_VIEW_H__
#define CANDIDATE_MC_GUI_SOLUTION_VIEW_H__

#include <crag/Crag.h>
#include <inference/CragSolution.h>
#include <scopegraph/Agent.h>
#include "Signals.h"

class SolutionView :
		public sg::Agent<
				SolutionView,
				sg::Accepts<
						SetCandidate,
						SetEdge
				>
		> {

public:

	SolutionView(
			const Crag&         crag,
			const CragSolution& solution,
			std::string         solutionName) :
		_crag(crag),
		_solution(solution),
		_solutionName(solutionName) {}

	void onSignal(SetCandidate& signal);

	void onSignal(SetEdge& signal);

private:

	const Crag&         _crag;
	const CragSolution& _solution;
	std::string         _solutionName;
};


#endif // CANDIDATE_MC_GUI_SOLUTION_VIEW_H__
