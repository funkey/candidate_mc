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

BestEffort::BestEffort(
		const Crag&                   crag,
		const CragVolumes&            volumes,
		const ExplicitVolume<int>&    groundTruth) :
	CragSolution(crag) {

	for (Crag::CragNode n : crag.nodes())
		setSelected(n, false);
	for (Crag::CragEdge e : crag.edges())
		setSelected(e, false);

	// assign leaf nodes to ground-truth regions

	Crag::NodeMap<std::map<int, int>> overlaps(crag);
	getGroundTruthOverlaps(crag, volumes, groundTruth, overlaps);

	Crag::NodeMap<int> gtAssignments(crag);
	getGroundTruthAssignments(crag, overlaps, gtAssignments);

	// recursively find the largest candidates assigned to only one ground-truth 
	// region
	Crag::NodeMap<std::set<int>> leafAssignments(crag);
	for (Crag::CragNode n : crag.nodes())
		if (crag.isRootNode(n))
			getLeafAssignments(crag, n, gtAssignments, leafAssignments);
	for (Crag::CragNode n : crag.nodes())
		if (crag.isRootNode(n))
			labelSingleAssignmentCandidate(crag, n, leafAssignments);

	// find all edges connecting switched on candidates assigned to the same 
	// ground-truth region
	for (Crag::CragEdge e : crag.edges()) {

		Crag::CragNode u = crag.u(e);
		Crag::CragNode v = crag.v(e);

		if (!selected(u) || !selected(v))
			continue;

		if (gtAssignments[u] != 0 && gtAssignments[u] == gtAssignments[v])
			setSelected(e, true);
	}
}

void
BestEffort::getGroundTruthOverlaps(
		const Crag&                        crag,
		const CragVolumes&                 volumes,
		const ExplicitVolume<int>&         groundTruth,
		Crag::NodeMap<std::map<int, int>>& overlaps) {

	for (Crag::CragNode n : crag.nodes()) {

		if (crag.type(n) == Crag::NoAssignmentNode)
			continue;

		const CragVolume& region = *volumes[n];

		util::point<unsigned int, 3> offset =
				(region.getOffset() - groundTruth.getOffset())/
				groundTruth.getResolution();

		for (unsigned int z = 0; z < region.getDiscreteBoundingBox().depth();  z++)
		for (unsigned int y = 0; y < region.getDiscreteBoundingBox().height(); y++)
		for (unsigned int x = 0; x < region.getDiscreteBoundingBox().width();  x++) {

			if (!region.data()(x, y, z))
				continue;

			int gtLabel = groundTruth[offset + util::point<unsigned int, 3>(x, y, z)];

			if (gtLabel == 0)
				continue;

			overlaps[n][gtLabel]++;
		}
	}
}

void
BestEffort::getGroundTruthAssignments(
		const Crag&                              crag,
		const Crag::NodeMap<std::map<int, int>>& overlaps,
		Crag::NodeMap<int>&                      gtAssignments) {

	for (Crag::CragNode i : crag.nodes()) {

		if (crag.type(i) == Crag::NoAssignmentNode)
			continue;

		// find most overlapping ground truth region
		int maxOverlap = 0;
		int bestGtLabel = 0;

		for (auto& p : overlaps[i]) {

			int gtLabel = p.first;
			int overlap = p.second;

			if (overlap > maxOverlap) {

				maxOverlap = overlap;
				bestGtLabel = gtLabel;
			}
		}

		gtAssignments[i] = bestGtLabel;
	}
}

void
BestEffort::getLeafAssignments(
		const Crag&                   crag,
		Crag::CragNode                n,
		const Crag::NodeMap<int>&     gtAssignments,
		Crag::NodeMap<std::set<int>>& leafAssignments) {

	leafAssignments[n].clear();

	// add all our children's assignments
	for (Crag::CragArc childArc : crag.inArcs(n)) {

		getLeafAssignments(crag, childArc.source(), gtAssignments, leafAssignments);
		leafAssignments[n].insert(leafAssignments[childArc.source()].begin(), leafAssignments[childArc.source()].end());
	}

	// add our own assignment
	leafAssignments[n].insert(gtAssignments[n]);
}

void
BestEffort::labelSingleAssignmentCandidate(
		const Crag&                         crag,
		Crag::CragNode                      n,
		const Crag::NodeMap<std::set<int>>& leafAssignments) {

	if (leafAssignments[n].size() == 1) {

		setSelected(n, true);
		return;
	}

	for (Crag::CragArc childArc : crag.inArcs(n))
		labelSingleAssignmentCandidate(crag, childArc.source(), leafAssignments);
}
