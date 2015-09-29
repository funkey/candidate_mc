#ifndef CANDIDATE_MC_LEARNING_ASSIGNMENT_LOSS_H__
#define CANDIDATE_MC_LEARNING_ASSIGNMENT_LOSS_H__

#include <imageprocessing/ExplicitVolume.h>
#include <learning/Loss.h>

/**
 * Specialized loss for assignment models. Rewards overlap between slices.
 */
class AssignmentLoss : public Loss {

public:

	AssignmentLoss(
			const Crag&                crag,
			const CragVolumes&         volumes,
			const ExplicitVolume<int>& groundTruth);

private:

	void computeSizes(
			const Crag& crag,
			const CragVolumes& volumes,
			const ExplicitVolume<int>& groundTruth);

	void computeOverlaps(
			const Crag& crag,
			const CragVolumes& volumes,
			const ExplicitVolume<int>& groundTruth);

	// get the size of the overlap of a candidate with any ground truth region 
	// it overlaps with
	std::map<int, int> overlap(
			const ExplicitVolume<bool>& region,
			const ExplicitVolume<int>&  groundTruth);

	std::map<int, int> _gtSizes;
	Crag::NodeMap<int> _candidateSizes;

	Crag::NodeMap<std::map<int, int>> _overlaps;
};

#endif // CANDIDATE_MC_LEARNING_ASSIGNMENT_LOSS_H__

