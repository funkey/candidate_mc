#ifndef CANDIDATE_MC_INFERENCE_CRAG_SOLVER_FACTORY_H__
#define CANDIDATE_MC_INFERENCE_CRAG_SOLVER_FACTORY_H__

#include "AssignmentSolver.h"
#include "MultiCutSolver.h"
#include "ClosedSetSolver.h"

class CragSolverFactory {

public:

	static CragSolver* createSolver(
			const Crag& crag,
			const CragVolumes& volumes,
			CragSolver::Parameters parameters = CragSolver::Parameters());
};

#endif // CANDIDATE_MC_INFERENCE_CRAG_SOLVER_FACTORY_H__

