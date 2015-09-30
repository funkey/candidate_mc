#include <inference/CragSolverFactory.h>
#include "BestEffort.h"

BestEffort::BestEffort(
		const Crag&                   crag,
		const CragVolumes&            volumes,
		const Costs&                  costs,
		const CragSolver::Parameters& params) :
	CragSolution(crag) {

	std::unique_ptr<CragSolver> solver(CragSolverFactory::createSolver(crag, volumes, params));
	solver->setCosts(costs);
	solver->solve(*this);
}
