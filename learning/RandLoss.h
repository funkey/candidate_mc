#ifndef CANDIDATE_MC_LEARNING_RAND_LOSS_H__
#define CANDIDATE_MC_LEARNING_RAND_LOSS_H__

#include <imageprocessing/ExplicitVolume.h>
#include <learning/Loss.h>

/**
 * Loss that approximates the RAND index of a solution compared to the ground 
 * truth.
 *
 * The loss of selecting a candidate is the number of incorrectly merged ground 
 * truth region voxel pairs, minus the number of correctly merged pairs. 
 * Similarly, the loss of selecting an adjacency edge is the number of correctly 
 * merged pairs between (but not within) the involved candidates, minus the 
 * number of incorrectly merged pairs.
 *
 * The loss of not selecting a candidate is minus the number of ground truth 
 * background voxel pairs covered by the candidate. I.e., there is a reward for 
 * not selecting a candidate based on the number of ground truth background 
 * voxels it overlaps with. The loss of not selecting an adjacency edge is the 
 * number of ground truth background voxel pairs between (but not within) the 
 * involved candidates.
 *
 * The losses for no selection are added negatively to the losses for selection. 
 * Note, that this loss can be negative.
 */
class RandLoss : public Loss {

public:

	RandLoss(
			const Crag&                crag,
			const CragVolumes&         volumes,
			const ExplicitVolume<int>& groundTruth);

private:

	void recurseRandLoss(
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

	double backgroundEdgeOverlapScore(
			const std::map<int, int>& overlapsU,
			const std::map<int, int>& overlapsV);

	Crag::NodeMap<std::map<int, int>> _overlaps;
};

#endif // CANDIDATE_MC_LEARNING_RAND_LOSS_H__

