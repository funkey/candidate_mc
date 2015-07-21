#include "BestEffort.h"

BestEffort::BestEffort(
		const Crag&                 crag,
		const CragVolumes&          volumes,
		const Costs&                costs,
		const MultiCut::Parameters& params) :
	node(crag),
	edge(crag) {

	MultiCut multicut(crag, params);
	multicut.setCosts(costs);
	multicut.solve();
	multicut.storeSolution(volumes, "best-effort.tif");
	multicut.storeSolution(volumes, "best-effort_boundary.tif", true);

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		node[n] = multicut.getSelectedRegions()[n];
	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e)
		edge[e] = multicut.getMergedEdges()[e];
}
