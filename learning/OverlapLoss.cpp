#include "OverlapLoss.h"
#include <util/Logger.h>
#include <util/assert.h>
#include <util/ProgramOptions.h>

logger::LogChannel overlaplosslog("overlaplosslog", "[OverlapLoss] ");

util::ProgramOption optionSetDifferenceWeight(
		util::_module           = "loss.overlap",
		util::_long_name        = "setDifferenceWeight",
		util::_description_text = "The influence of voxels that are neither in the candidate nor in the ground-truth "
		                          "region to the loss. The loss is <set difference>*<set difference weight> - <overlap>. "
		                          "Default is 1.",
		util::_default_value    = 1.0);

OverlapLoss::OverlapLoss(
		const Crag&                crag,
		const CragVolumes&         volumes,
		const ExplicitVolume<int>& groundTruth) :
	Loss(crag),
	_candidateSizes(crag),
	_overlaps(crag) {

	computeSizesAndOverlaps(crag, volumes, groundTruth);

	// For each candidate i, get the gt region j with maximal overlap and set
	//
	//   score = difference_i_to_j*w - overlap_i_and_j
	//
	// where
	//
	//   differnce_i_to_j = size_of_i + size_of_j - 2*overlap_i_and_j
	//
	// are all pixels in i and not in j and vice versa. Hence,
	//
	//   score =
	//        size_of_i*w + size_of_j*w - (2*w + 1)*overlap_i_and_j

	double w = optionSetDifferenceWeight.as<double>();

	for (Crag::CragNode i : crag.nodes()) {

		if (crag.type(i) == Crag::NoAssignmentNode) {

			// NoAssignmentNodes don't have a loss
			node[i] = 0;
			continue;
		}

		LOG_ALL(overlaplosslog) << "computing loss for node " << crag.id(i) << std::endl;

		int size_i = _candidateSizes[i];

		// find most overlapping ground truth region
		int bestGtSize = 0;
		int maxOverlap = 0;

		for (auto& p : _overlaps[i]) {

			int gtLabel = p.first;
			int overlap = p.second;
			int size_j  = _gtSizes[gtLabel];

			LOG_ALL(overlaplosslog) << "\toverlap with  gt region " << gtLabel << ": " << overlap << std::endl;
			LOG_ALL(overlaplosslog) << "\tdifference to gt region " << gtLabel << ": " << (size_i - overlap) << std::endl;

			if (overlap > maxOverlap) {

				maxOverlap = overlap;
				bestGtSize = size_j;
			}
		}

		node[i] = size_i*w + bestGtSize*w - (2*w + 1)*maxOverlap;
	}

	// edges don't have a loss
	for (Crag::CragEdge e : crag.edges())
		edge[e] = 0;
}

void
OverlapLoss::computeSizesAndOverlaps(
		const Crag& crag,
		const CragVolumes& volumes,
		const ExplicitVolume<int>& groundTruth) {

	// ground truth sizes
	_gtSizes.clear();
	for (int l : groundTruth.data())
		if (l > 0)
			_gtSizes[l]++;

	// candidate sizes and overlap with ground truth regions
	for (Crag::CragNode n : crag.nodes()) {

		_candidateSizes[n] = 0;

		if (crag.type(n) == Crag::NoAssignmentNode)
			continue;

		const CragVolume& region = *volumes[n];

		util::point<unsigned int, 3> offset =
				(region.getOffset() - groundTruth.getOffset())/
				groundTruth.getResolution();

		LOG_ALL(overlaplosslog) << "offset into ground-truth image: " << offset << std::endl;

		for (unsigned int z = 0; z < region.getDiscreteBoundingBox().depth();  z++)
		for (unsigned int y = 0; y < region.getDiscreteBoundingBox().height(); y++)
		for (unsigned int x = 0; x < region.getDiscreteBoundingBox().width();  x++) {

			if (!region.data()(x, y, z))
				continue;

			_candidateSizes[n]++;

			int gtLabel = groundTruth[offset + util::point<unsigned int, 3>(x, y, z)];

			if (gtLabel == 0)
				continue;

			_overlaps[n][gtLabel]++;
		}
	}
}

