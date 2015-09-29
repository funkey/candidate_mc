#ifndef CANDIDATE_MC_INFERENCE_SOLVER_H__
#define CANDIDATE_MC_INFERENCE_SOLVER_H__

#include <crag/Crag.h>
#include "Costs.h"

/**
 * Interface for CRAG solvers.
 */
class Solver {

public:

	struct Parameters {

		Parameters() :
			forceExplanation(false),
			numIterations(100),
			maxConstraintsPerIteration(0),
			noConstraints(false),
			minimize(true) {}

		/**
		 * If true, force exactly one region to be chosen for each root-to-leaf 
		 * path in the subset tree of the CRAG. This implies that there will be 
		 * no background region.
		 */
		bool forceExplanation;

		/**
		 * The maximal number of iterations to solve. Not used by 
		 * AssignmentSolver.
		 */
		int numIterations;

		/**
		 * The maximal number of cycle constraints to add per iteration. Not 
		 * used by AssignmentSolver.
		 */
		int maxConstraintsPerIteration;

		/**
		 * Disable all constraints (conflict constraints on candidates, 
		 * rejection constraints, path constraints). This basically solves a 
		 * "thresholding" relaxation of the original problem. Not used by 
		 * AssignmentSolver.
		 */
		bool noConstraints;

		bool minimize;
	};

	enum Status {

		SolutionFound,

		MaxIterationsReached
	};

	Solver(const Crag& crag) :
		_merged(crag),
		_selected(crag),
		_labels(crag) {}

	/**
	 * Set the costs (or reward, if negative) of accepting a node or an edge.
	 */
	virtual void setCosts(const Costs& costs) = 0;

	virtual Status solve() = 0;

	/**
	 * Get the current solution in terms of edges that have been selected to 
	 * merge regions. If solve() did not return SolutionFound, the solution 
	 * might be suboptimal and/or inconsistent.
	 */
	const Crag::EdgeMap<bool>& getMergedEdges() const { return _merged; }

	/**
	 * Get the current solution in terms of selected regions.If solve() did not 
	 * return SolutionFound, the solution might be suboptimal.
	 */
	const Crag::NodeMap<bool>& getSelectedRegions() const { return _selected; }

	/**
	 * Get the current solution in terms of a connected component labelling: 
	 * Every region will be labelled with an id representing the connected 
	 * component it belongs to. This label will also be set for subregions of 
	 * selected regions. If solve() did not return SolutionFound, the solution 
	 * might be suboptimal.
	 */
	const Crag::NodeMap<int>& getLabels() const { return _labels; }

	/**
	 * Get the value of the current solution.
	 */
	virtual double getValue() = 0;

protected:

	Crag::EdgeMap<bool> _merged;
	Crag::NodeMap<bool> _selected;
	Crag::NodeMap<int>  _labels;
};

#endif // CANDIDATE_MC_INFERENCE_SOLVER_H__

