#ifndef CANDIDATE_MC_INFERENCE_ASSIGNMENT_SOLVER_H__
#define CANDIDATE_MC_INFERENCE_ASSIGNMENT_SOLVER_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <solver/LinearSolverBackend.h>
#include "Costs.h"
#include "CragSolver.h"

class AssignmentSolver : public CragSolver {

public:

	AssignmentSolver(
			const Crag& crag,
			const CragVolumes& volumes,
			const Parameters& parameters = Parameters());

	/**
	 * Set the costs (or reward, if negative) of accepting a node or an edge.
	 */
	void setCosts(const Costs& costs) override;

	Status solve() override;

	/**
	 * Get the value of the current solution.
	 */
	double getValue() override { return _solution.getValue(); }

private:

	void checkCrag();

	void prepareSolver();

	void setVariables();

	void setConstraints();

	int collectTreePathConstraints(Crag::CragNode n, std::vector<int>& pathIds);

	void findAssignments();

	void propagateLabel(Crag::SubsetNode n, int label);

	inline unsigned int nodeIdToVar(int nodeId) { return nodeId; }
	inline unsigned int edgeIdToVar(int edgeId) { return _edgeIdToVarMap[edgeId]; }

	const Crag& _crag;
	const CragVolumes& _volumes;

	unsigned int _numNodes, _numEdges;

	std::map<int, unsigned int> _edgeIdToVarMap;

	LinearObjective                      _objective;
	LinearConstraints                    _constraints;
	std::unique_ptr<LinearSolverBackend> _solver;
	Solution                             _solution;

	Parameters _parameters;
};

#endif // CANDIDATE_MC_INFERENCE_ASSIGNMENT_SOLVER_H__

