#ifndef CANDIDATE_MC_LEARNING_BEST_EFFORT_H__
#define CANDIDATE_MC_LEARNING_BEST_EFFORT_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <inference/CragSolver.h>
#include "CragSolution.h"

class BestEffort : public CragSolution {

public:

	/**
	 * Crate a new uninitialized best-effort solution.
	 */
	BestEffort(const Crag& crag) : CragSolution(crag) {}

	/**
	 * Create a best-effort solution by solving the CRAG with the given costs.
	 */
	BestEffort(
			const Crag&                   crag,
			const CragVolumes&            volumes,
			const Costs&                  costs,
			const CragSolver::Parameters& params = CragSolver::Parameters());

	/**
	 * Create a best-effort solution by assigning each leaf candidate to the 
	 * ground-truth region with maximal overlap. The best-effort candidates are 
	 * the largest candidates whose leafs are only assigned to one region 
	 * (excluding the background). Adjacency edges are switched on between 
	 * candidates with the same assignment.
	 */
	BestEffort(
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

	void getLeafAssignments(
			const Crag&                   crag,
			Crag::CragNode                n,
			const Crag::NodeMap<int>&     gtAssignments,
			Crag::NodeMap<std::set<int>>& leafAssignments);
	
	void findConcordantLeafNodeCandidates(
			const Crag&               crag,
			const Crag::NodeMap<int>& gtAssignments);
	
	void findMajorityOverlapCandidates(
			const Crag&                              crag,
			const Crag::NodeMap<std::map<int, int>>& overlaps,
			const Crag::NodeMap<int>&                gtAssignments);

	void labelSingleAssignmentCandidate(
			const Crag&                         crag,
			Crag::CragNode                      n,
			const Crag::NodeMap<std::set<int>>& leafAssignments);
	
	void labelMajorityOverlapCandidate(
			const Crag&                              crag,
			const Crag::CragNode&                    n,
			const Crag::NodeMap<std::map<int, int>>& overlaps,
			const Crag::NodeMap<int>&                gtAssignments);

	// include children and child edges of best-effort candidates and edges
	bool _fullBestEffort;

	// when considering overlap with gt regions, scale the overlap with 
	// background by this value
	double _bgOverlapWeight;

	bool _onlySliceNode;
};

#endif // CANDIDATE_MC_LEARNING_BEST_EFFORT_H__
