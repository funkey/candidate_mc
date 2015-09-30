#ifndef CANDIDATE_MC_INFERENCE_SOLVER_H__
#define CANDIDATE_MC_INFERENCE_SOLVER_H__

#include <crag/Crag.h>
#include "Costs.h"
#include "CragSolution.h"

/**
 * Interface for CRAG solvers.
 */
class CragSolver {

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

	/**
	 * Set the costs (or reward, if negative) of accepting a node or an edge.
	 */
	virtual void setCosts(const Costs& costs) = 0;

	/**
	 * Get the current solution of the solver. If solve() does not return 
	 * SolutionFound, the solution might be suboptimal.
	 */
	virtual Status solve(CragSolution& solution) = 0;

	/**
	 * Get the value of the current solution.
	 */
	virtual double getValue() = 0;
};

#endif // CANDIDATE_MC_INFERENCE_SOLVER_H__

