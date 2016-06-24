#ifndef CANDIDATE_MC_CLOSED_SET_SOLVER_H__
#define CANDIDATE_MC_CLOSED_SET_SOLVER_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <solver/LinearSolverBackend.h>
#include "Costs.h"
#include "CragSolver.h"

class ClosedSetSolver : public CragSolver {

public:

	ClosedSetSolver(const Crag& crag, const Parameters& parameters = Parameters());

	~ClosedSetSolver();

	/**
	 * Set the costs (or reward, if negative) of accepting a node or an edge.
	 */
	void setCosts(const Costs& costs) override;

	Status solve(CragSolution& solution) override;

	/**
	 * Get the value of the current solution.
	 */
	double getValue() override { return _solution.getValue(); }

private:

	void prepareSolver();

	void setVariables();

	void setInitialConstraints();

	void findMinClosedSet(CragSolution& solution);

	bool findViolatedConstraints(CragSolution& solution);

	inline unsigned int nodeIdToVar(int nodeId) { return nodeId; }
	inline unsigned int edgeIdToVar(int edgeId) { return _edgeIdToVarMap[edgeId]; }

	const Crag& _crag;

	unsigned int _numNodes, _numEdges;

	std::map<int, unsigned int> _edgeIdToVarMap;

	LinearObjective      _objective;
	LinearConstraints    _constraints;
	LinearSolverBackend* _solver;
	Solution             _solution;

	Parameters _parameters;
};

#endif // CANDIDATE_MC_CLOSED_SET_SOLVER_H__

