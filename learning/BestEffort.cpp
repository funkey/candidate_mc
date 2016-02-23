#include <inference/CragSolverFactory.h>
#include <util/ProgramOptions.h>
#include "BestEffort.h"

util::ProgramOption optionBackgroundOverlapWeight(
		util::_long_name        = "backgroundOverlapWeight",
		util::_description_text = "The weight of background voxels for the computation of the best-effort. A value smaller than 1 means "
		                          "that a supervoxel can be assigned to a ground-truth region even though it overlaps with more than 50% "
		                          "with background.",
		util::_default_value    = 1);

util::ProgramOption optionMajorityOverlap(
		util::_module           = "best-effort",
		util::_long_name        = "majorityOverlap",
		util::_description_text = "Switch to an alternative strategy to find the best-effort solution. If set, the largest candidate that "
		                          "has a majority overlap with a ground-truth region will be selected and assigned to this region. If none "
		                          "of the candidates along a path has a majority overlap, the leaf node is selected and assigned to the ground-"
		                          "truth region with maximal overlap. If two adjacent candidates are selected and assigned to the same ground-"
		                          "truth region, the adjacency edge is also selected. If this option is not set, the largest candidate that "
		                          "has leaf nodes that are all assigned to the same ground-truth region is selected and assigned to this region.");

BestEffort::BestEffort(
		const Crag&                   crag,
		const CragVolumes&            volumes,
		const Costs&                  costs,
		const CragSolver::Parameters& params) :
	CragSolution(crag),
	_bgOverlapWeight(optionBackgroundOverlapWeight) {

	std::unique_ptr<CragSolver> solver(CragSolverFactory::createSolver(crag, volumes, params));
	solver->setCosts(costs);
	solver->solve(*this);
}

BestEffort::BestEffort(
		const Crag&                   crag,
		const CragVolumes&            volumes,
		const ExplicitVolume<int>&    groundTruth) :
	CragSolution(crag),
	_bgOverlapWeight(optionBackgroundOverlapWeight) {

	for (Crag::CragNode n : crag.nodes())
		setSelected(n, false);
	for (Crag::CragEdge e : crag.edges())
		setSelected(e, false);

	// assign each candidate to the ground-truth region with maximal overlap (this does not select the candidates, yet)

	Crag::NodeMap<std::map<int, int>> overlaps(crag);
	getGroundTruthOverlaps(crag, volumes, groundTruth, overlaps);

	Crag::NodeMap<int> gtAssignments(crag);
	getGroundTruthAssignments(crag, overlaps, gtAssignments);

	// recursively find the largest candidates assigned to only one ground-truth 
	// region
	if (optionMajorityOverlap)
		findMajorityOverlapCandidates(crag, overlaps, gtAssignments);
	else
		findConcordantLeafNodeCandidates(crag, gtAssignments);

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
		double maxOverlap = 0;
		int bestGtLabel = 0;

		for (auto& p : overlaps[i]) {

			int gtLabel = p.first;
			double overlap = p.second;

			if (gtLabel == 0)
				overlap *= _bgOverlapWeight;

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
BestEffort::findMajorityOverlapCandidates(
		const Crag&                              crag,
		const Crag::NodeMap<std::map<int, int>>& overlaps,
		const Crag::NodeMap<int>&                gtAssignments) {
  
	for (Crag::CragNode n : crag.nodes())
		if (crag.isRootNode(n))
			labelMajorityOverlapCandidate(crag, n, overlaps, gtAssignments);
}

void
BestEffort::findConcordantLeafNodeCandidates(
		const Crag&                              crag,
		const Crag::NodeMap<int>&                gtAssignments) {

	Crag::NodeMap<std::set<int>> leafAssignments(crag);
	for (Crag::CragNode n : crag.nodes())
		if (crag.isRootNode(n))
			getLeafAssignments(crag, n, gtAssignments, leafAssignments);
	for (Crag::CragNode n : crag.nodes())
		if (crag.isRootNode(n))
			labelSingleAssignmentCandidate(crag, n, leafAssignments);
}

void
BestEffort::labelMajorityOverlapCandidate(
		const Crag&                              crag,
		const Crag::CragNode&                    n,
		const Crag::NodeMap<std::map<int, int>>& overlaps,
		const Crag::NodeMap<int>&                gtAssignments) {

	double maxOverlap = overlaps[n].at(gtAssignments[n]);
	if (gtAssignments[n] == 0)
		maxOverlap *= _bgOverlapWeight;

	double totalOverlap = 0;
	for (auto& p : overlaps[n])
		totalOverlap += ((double)p.second)*(p.first == 0 ? _bgOverlapWeight : 1.0);
 
	if (crag.isLeafNode(n) || maxOverlap/totalOverlap > 0.5) {

		setSelected(n, gtAssignments[n] != 0);
		return;
	}

	for (Crag::CragArc childArc : crag.inArcs(n))
		labelMajorityOverlapCandidate(crag, childArc.source(), overlaps, gtAssignments);
}

void
BestEffort::labelSingleAssignmentCandidate(
		const Crag&                         crag,
		Crag::CragNode                      n,
		const Crag::NodeMap<std::set<int>>& leafAssignments) {

	if (leafAssignments[n].size() == 1 && (*leafAssignments[n].begin()) != 0) {

		setSelected(n, true);
		return;
	}

	for (Crag::CragArc childArc : crag.inArcs(n))
		labelSingleAssignmentCandidate(crag, childArc.source(), leafAssignments);
}
