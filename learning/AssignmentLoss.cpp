#include "AssignmentLoss.h"
#include <util/Logger.h>
#include <util/assert.h>

logger::LogChannel assignmentlosslog("assignmentlosslog", "[AssignmentLoss] ");

AssignmentLoss::AssignmentLoss(
		const Crag&                crag,
		const CragVolumes&         volumes,
		const ExplicitVolume<int>& groundTruth) :
	Loss(crag),
	_candidateSizes(crag),
	_overlaps(crag) {

	computeSizesAndOverlaps(crag, volumes, groundTruth);

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

		if (crag.type(i) == Crag::SliceNode) {

			// SliceNodes don't need a score, their selection is implied by 
			// selecting AssignmentNodes. However, if they don't overlap with a 
			// ground truth region at all, discourage taking them.
			if (_candidateSizes[i] == 0)
				node[i] = 1;
			else
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

			LOG_ALL(assignmentlosslog) << "\toverlap with  gt region " << gtLabel << ": " << overlap << std::endl;
			LOG_ALL(assignmentlosslog) << "\tdifference to gt region " << gtLabel << ": " << (size_i - overlap) << std::endl;

			int score = size_i - 2*overlap;

			LOG_ALL(assignmentlosslog) << "\tscore with    gt region " << gtLabel << ": " << score << std::endl;

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
AssignmentLoss::computeSizesAndOverlaps(
		const Crag& crag,
		const CragVolumes& volumes,
		const ExplicitVolume<int>& groundTruth) {

	// candidate sizes and overlap with ground truth regions
	for (Crag::CragNode n : crag.nodes()) {

		_candidateSizes[n] = 0;

		if (crag.type(n) == Crag::NoAssignmentNode)
			continue;

		const CragVolume& region = *volumes[n];

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

			if (gtLabel == 0)
				continue;

			// For the size of the candidates, consider only voxels that do 
			// overlap with a ground truth region. This way, we say that we 
			// don't care about the background label in the ground truth.
			_candidateSizes[n]++;

			_overlaps[n][gtLabel]++;
		}
	}
}

