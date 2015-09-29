#include "AssignmentLoss.h"
#include <util/Logger.h>

logger::LogChannel assignmentlosslog("assignmentlosslog", "[AssignmentLoss] ");

AssignmentLoss::AssignmentLoss(
		const Crag&                crag,
		const CragVolumes&         volumes,
		const ExplicitVolume<int>& groundTruth) :
	Loss(crag),
	_candidateSizes(crag),
	_overlaps(crag) {

	computeSizes(crag, volumes, groundTruth);
	computeOverlaps(crag, volumes, groundTruth);

	// For each candidate i (SliceNode and AssignmentNode), get the minimal
	//
	//   score = difference_i_to_j - overlap_i_and_j
	//
	// over all ground truth regions j as costs.
	//
	//   differnce_i_to_j = size_of_i - overlap_i_and_j
	//
	// are all pixels in i and not in j, Hence,
	//
	//   score = size_of_i - 2 * overlap_i_and_j

	for (Crag::CragNode i : crag.nodes()) {

		if (crag.type(i) == Crag::NoAssignmentNode) {

			// NoAssignmentNodes don't have a loss
			node[i] = 0;
			continue;
		}

		LOG_ALL(assignmentlosslog) << "computing loss for node " << crag.id(i) << std::endl;

		int size_i = _candidateSizes[i];
		int minScore = size_i;

		// for each overlapping ground truth region
		for (auto& p : _overlaps[i]) {

			int gtLabel = p.first;
			int overlap = p.second;

			LOG_ALL(assignmentlosslog) << "\toverlap gt region " << gtLabel << ": " << overlap << std::endl;

			int score = size_i - 2*overlap;

			if (score < minScore)
				minScore = score;
		}

		node[i] = minScore;
	}

	// edges don't have a loss
	for (Crag::CragEdge e : crag.edges())
		edge[e] = 0;
}

void
AssignmentLoss::computeSizes(
		const Crag& crag,
		const CragVolumes& volumes,
		const ExplicitVolume<int>& groundTruth) {

	_gtSizes.clear();

	// ground truth sizes
	for (int l : groundTruth.data())
		if (l > 0)
			_gtSizes[l]++;

	for (Crag::CragNode n : crag.nodes())
		_candidateSizes[n] = 0;

	// candidate sizes
	for (Crag::CragNode n : crag.nodes())
		for (int l : volumes[n]->data())
			if (l)
				_candidateSizes[n]++;
}

void
AssignmentLoss::computeOverlaps(
		const Crag& crag,
		const CragVolumes& volumes,
		const ExplicitVolume<int>& groundTruth) {

	for (Crag::CragNode n : crag.nodes()) {

		if (crag.type(n) == Crag::NoAssignmentNode)
			continue;

		_overlaps[n] = overlap(*volumes[n], groundTruth);
	}
}

std::map<int, int>
AssignmentLoss::overlap(
		const ExplicitVolume<bool>& region,
		const ExplicitVolume<int>&  groundTruth) {

	std::map<int, int> overlaps;

	util::point<unsigned int, 3> offset =
			(region.getOffset() - groundTruth.getOffset())/
			groundTruth.getResolution();

	LOG_ALL(assignmentlosslog) << "offset into ground-truth image: " << offset << std::endl;

	for (unsigned int z = 0; z < region.getDiscreteBoundingBox().depth();  z++)
	for (unsigned int y = 0; y < region.getDiscreteBoundingBox().height(); y++)
	for (unsigned int x = 0; x < region.getDiscreteBoundingBox().width();  x++) {

		if (!region.data()(x, y, z))
			continue;

		int gtLabel = groundTruth[offset + util::point<unsigned int, 3>(x, y, z)];

		if (!overlaps.count(gtLabel))
			overlaps[gtLabel] = 1;
		else
			overlaps[gtLabel]++;
	}

	return overlaps;
}
