#ifndef CANDIDATE_MC_LEARNING_BEST_EFFORT_LOSS_H__
#define CANDIDATE_MC_LEARNING_BEST_EFFORT_LOSS_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <learning/Loss.h>

class BestEffortLoss : public Loss {

public:

	BestEffortLoss(
			const Crag&                   crag,
			const CragVolumes&            volumes,
			const ExplicitVolume<int>&    groundTruth);

private:

	void getGroundTruthOverlaps(
			const Crag&                        crag,
			const CragVolumes&                 volumes,
			const ExplicitVolume<int>&         groundTruth,
			Crag::NodeMap<std::map<int, int>>& overlaps);

	void getGroundTruthAssignments(
			const Crag&                              crag,
			const Crag::NodeMap<std::map<int, int>>& overlaps,
			Crag::NodeMap<int>&                      gtAssignments);

	void findConcordantLeafNodeCandidates(
			const Crag&               crag,
			const Crag::NodeMap<int>& gtAssignments);

	void getLeafAssignments(
			const Crag&                   crag,
			Crag::CragNode                n,
			const Crag::NodeMap<int>&     gtAssignments,
			Crag::NodeMap<std::set<int>>& leafAssignments);

	void labelSingleAssignmentCandidate(
			const Crag&                         crag,
			Crag::CragNode                      n,
			const Crag::NodeMap<std::set<int>>& leafAssignments);

	void setAssignmentCost(
			const Crag&                        crag,
			const CragVolumes&                 volumes,
			const ExplicitVolume<int>&         groundTruth,
			Crag::NodeMap<int>&                gtAssignments,
			Crag::NodeMap<std::map<int, int>>& overlaps);

	void unselectChildren(
			const Crag&    crag,
			Crag::CragNode n);
};

#endif // CANDIDATE_MC_LEARNING_BEST_EFFORT_H__
