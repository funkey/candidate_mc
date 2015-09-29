#include <inference/CragSolverFactory.h>
#include "BestEffort.h"

BestEffort::BestEffort(
		const Crag&               crag,
		const CragVolumes&        volumes,
		const Costs&              costs,
		const Solver::Parameters& params) :
	node(crag),
	edge(crag) {

	std::unique_ptr<Solver> solver(CragSolverFactory::createSolver(crag, volumes, params));
	solver->setCosts(costs);
	solver->solve();
	//solver->storeSolution(volumes, "best-effort.tif");
	//solver->storeSolution(volumes, "best-effort_boundary.tif", true);

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		node[n] = solver->getSelectedRegions()[n];
	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e)
		edge[e] = solver->getMergedEdges()[e];
}
