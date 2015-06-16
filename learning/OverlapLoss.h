#ifndef CANDIDATE_MC_LEARNING_OVERLAP_COSTS_H__
#define CANDIDATE_MC_LEARNING_OVERLAP_COSTS_H__

#include <inference/Costs.h>

/**
 * Costs that reflect the overlap of each region with the ground truth.
 */
class OverlapCosts : public Costs {

public:

	OverlapCosts(
			const Crag&                crag,
			const ExplicitVolume<int>& groundTruth);

private:

	void recurseOverlapCosts(
			const Crag&                crag,
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

#endif // CANDIDATE_MC_LEARNING_OVERLAP_COSTS_H__

