#ifndef CANDIDATE_MC_LEARNING_OVERLAP_LOSS_H__
#define CANDIDATE_MC_LEARNING_OVERLAP_LOSS_H__

#include <imageprocessing/ExplicitVolume.h>
#include <learning/Loss.h>

/**
 * Loss that reflect the overlap of each region with the ground truth.
 */
class OverlapLoss : public Loss {

public:

	OverlapLoss(
			const Crag&                crag,
			const CragVolumes&         volumes,
			const ExplicitVolume<int>& groundTruth);

private:

	void recurseOverlapLoss(
			const Crag&                crag,
			const CragVolumes&         volumes,
			const Crag::Node&          n,
			const ExplicitVolume<int>& groundTruth);

	std::map<int, int> leafOverlaps(
			const ExplicitVolume<bool>& region,
			const ExplicitVolume<int>&  groundTruth);

	double foregroundNodeOverlapScore(
			const std::map<int, int>& overlaps);

	double foregroundEdgeOverlapScore(
			const std::map<int, int>& overlapsU,
			const std::map<int, int>& overlapsV);

	double backgroundNodeOverlapScore(
			const std::map<int, int>& overlaps);

	Crag::NodeMap<std::map<int, int>> _overlaps;
};

#endif // CANDIDATE_MC_LEARNING_OVERLAP_LOSS_H__

