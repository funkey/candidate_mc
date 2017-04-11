#include <util/Logger.h>
#include "BestEffortLoss.h"

logger::LogChannel bestEffortLosslog("bestEffortLosslog", "[BestEffortLoss] ");

BestEffortLoss::BestEffortLoss(
		const Crag&                   crag,
		const CragVolumes&            volumes,
		const ExplicitVolume<int>&    groundTruth) :
		Loss(crag){

	// Set a positive cost for all slice nodes (no one is previously selected)
	// and zero for (no) assignment nodes
	for (Crag::CragNode n : crag.nodes()) {
		node[n] = 0;

		if (crag.type(n) == Crag::SliceNode)
			node[n] = +1;
	}

	// assign each candidate to the ground-truth region with maximal overlap
	Crag::NodeMap<std::map<int, int>> overlaps(crag);
	getGroundTruthOverlaps(crag, volumes, groundTruth, overlaps);

	Crag::NodeMap<int> gtAssignments(crag);
	getGroundTruthAssignments(crag, overlaps, gtAssignments);

	findConcordantLeafNodeCandidates(crag, gtAssignments);

	// define costs for the assignment nodes
	setAssignmentCost(crag, volumes, groundTruth, gtAssignments, overlaps);
}

void
BestEffortLoss::getGroundTruthOverlaps(
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
BestEffortLoss::getGroundTruthAssignments(
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

			if (overlap > maxOverlap) {

				maxOverlap = overlap;
				bestGtLabel = gtLabel;
			}
		}

		gtAssignments[i] = bestGtLabel;
		LOG_ALL(bestEffortLosslog) << "node: " << crag.id(i) << " label: " << bestGtLabel << std::endl;
	}
}

void
BestEffortLoss::findConcordantLeafNodeCandidates(
		const Crag&               crag,
		const Crag::NodeMap<int>& gtAssignments) {

	Crag::NodeMap<std::set<int>> leafAssignments(crag);

	for (Crag::CragNode n : crag.nodes())
		if (crag.isRootNode(n))
			getLeafAssignments(crag, n, gtAssignments, leafAssignments);

	for (Crag::CragNode n : crag.nodes())
		if (crag.isRootNode(n))
			labelSingleAssignmentCandidate(crag, n, leafAssignments);
}

void
BestEffortLoss::getLeafAssignments(
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
BestEffortLoss::labelSingleAssignmentCandidate(
		const Crag&                         crag,
		Crag::CragNode                      n,
		const Crag::NodeMap<std::set<int>>& leafAssignments) {

	if (leafAssignments[n].size() == 1 && (*leafAssignments[n].begin()) != 0)
		// For the highest slice node with a single assignment, set its cost to -1 (to be selected)
		if (crag.type(n) == Crag::SliceNode)
			node[n] = -1;

	for (Crag::CragArc childArc : crag.inArcs(n))
		labelSingleAssignmentCandidate(crag, childArc.source(), leafAssignments);
}

void BestEffortLoss::setAssignmentCost(
		const Crag&                 crag,
		const CragVolumes&          volumes,
		const ExplicitVolume<int>&  groundTruth,
		Crag::NodeMap<int>&         gtAssignments,
		Crag::NodeMap<std::map<int, int>>& overlaps)
{
	// for each slice node, if a parent must be selected, set the cost of the children as a higher cost
	for (Crag::CragNode n : crag.nodes()) {

		if (crag.type(n) != Crag::SliceNode)
			continue;

		if (node[n] == -1)
			unselectChildren(crag, n);
	}

	// For all assignment nodes, check if it links selected candidates with the same label
	for (Crag::CragNode n : crag.nodes()) {

		if (crag.type(n) != Crag::AssignmentNode)
			continue;

		bool selectAssignmentNode = true;
		int sliceLabel = -1;

		for (Crag::CragEdge edge : crag.adjEdges(n)) {

			Crag::CragNode opposite = crag.oppositeNode(n, edge);

			UTIL_ASSERT_REL(crag.type(opposite), ==, Crag::SliceNode)

			// If the candidate (slice node) is not to be select, go to the next assignment node
			if (node[opposite] != -1) {

				selectAssignmentNode = false;
				break;
			}

			if (sliceLabel == -1) {
				// first slice node
				sliceLabel = gtAssignments[opposite];
			} else {

				// subsequent slice node, if different label than first one,
				// don't take this assignment node
				if (sliceLabel != gtAssignments[opposite]) {
					selectAssignmentNode = false;
					break;
				}
			}
		}

		// if the assignment node links candidates with same label, set its cost as the negative overlap
		if (selectAssignmentNode) {
			node[n] = -(overlaps[n][sliceLabel]);

			LOG_ALL(bestEffortLosslog) << "Setting assignment node "<< crag.id(n) << "with cost" << node[n] << std::endl;
		}
	}
}

void BestEffortLoss::unselectChildren(
		const Crag&    crag,
		Crag::CragNode n)
{
	for (Crag::CragArc arc : crag.inArcs(n)) {
		node[arc.source()] = 1;
		unselectChildren(crag, arc.source());
	}
}

