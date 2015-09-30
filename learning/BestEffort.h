#ifndef CANDIDATE_MC_LEARNING_BEST_EFFORT_H__
#define CANDIDATE_MC_LEARNING_BEST_EFFORT_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <inference/CragSolver.h>
#include "CragSolution.h"

class BestEffort : public CragSolution {

public:

	/**
	 * Crate a new uninitialized best-effort solution.
	 */
	BestEffort(const Crag& crag) : CragSolution(crag) {}

	/**
	 * Create a best-effort solution by solving the CRAG with the given costs.
	 */
	BestEffort(
			const Crag&                   crag,
			const CragVolumes&            volumes,
			const Costs&                  costs,
			const CragSolver::Parameters& params = CragSolver::Parameters());
};

#endif // CANDIDATE_MC_LEARNING_BEST_EFFORT_H__

