#include <inference/CragSolverFactory.h>
#include "BestEffort.h"

BestEffort::BestEffort(
		const Crag&                   crag,
		const CragVolumes&            volumes,
		const Costs&                  costs,
		const CragSolver::Parameters& params) :
	node(crag),
	edge(crag) {

	std::unique_ptr<CragSolver> solver(CragSolverFactory::createSolver(crag, volumes, params));
	solver->setCosts(costs);
	solver->solve();
	//solver->storeSolution(volumes, "best-effort.tif");
	//solver->storeSolution(volumes, "best-effort_boundary.tif", true);

	for (Crag::CragNode n : crag.nodes())
		node[n] = solver->getSolution().selected(n);
	for (Crag::CragEdge e : crag.edges())
		edge[e] = solver->getSolution().selected(e);
}
