#ifndef CANDIDATE_MC_LEARNING_OVERLAP_LOSS_H__
#define CANDIDATE_MC_LEARNING_OVERLAP_LOSS_H__

#include <imageprocessing/ExplicitVolume.h>
#include <learning/Loss.h>

/**
 * Simple overlap-based loss for candidates. The score for selecting a candidate 
 * i is the minimal
 *
 *   difference_i_to_j - overlap_i_and_j
 *
 * to any ground truth region j, where difference_i_to_j is the number of pixels 
 * in i and not in j or in j and not in i.
 */
class OverlapLoss : public Loss {

public:

	OverlapLoss(
			const Crag&                crag,
			const CragVolumes&         volumes,
			const ExplicitVolume<int>& groundTruth);

private:

	void computeSizesAndOverlaps(
			const Crag& crag,
			const CragVolumes& volumes,
			const ExplicitVolume<int>& groundTruth);

	std::map<int, int> _gtSizes;
	Crag::NodeMap<int> _candidateSizes;

	Crag::NodeMap<std::map<int, int>> _overlaps;
};

#endif // CANDIDATE_MC_LEARNING_OVERLAP_LOSS_H__

