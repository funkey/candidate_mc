#ifndef CANDIDATE_MC_SOLVER_MULTI_CUT_H__
#define CANDIDATE_MC_SOLVER_MULTI_CUT_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <solver/LinearSolverBackend.h>
#include <vigra/tinyvector.hxx>
#include "Costs.h"
#include "Solver.h"

class MultiCutSolver : public Solver {

public:

	MultiCutSolver(const Crag& crag, const Parameters& parameters = Parameters());

	~MultiCutSolver();

	/**
	 * Set the costs (or reward, if negative) of accepting a node or an edge.
	 */
	void setCosts(const Costs& costs) override;

	Status solve() override;

	/**
	 * Get the value of the current solution.
	 */
	double getValue() override { return _solution.getValue(); }

	/**
	 * Store the solution as label image in the given image file.
	 */
	void storeSolution(const CragVolumes& volumes, const std::string& filename, bool drawBoundary = false);

private:
    typedef vigra::TinyVector<unsigned int, 3> TinyVector3UInt;
	// a property map returning 1 for every entry
	struct One {

		typedef int Value;

		template <typename T>
		int operator[](const T&) const { return 1; }

	};

	void prepareSolver();

	void setVariables();

	void setInitialConstraints();

	int collectTreePathConstraints(Crag::SubsetNode n, std::vector<int>& pathIds);

	void findCut();

	bool findViolatedConstraints();

	void propagateLabel(Crag::SubsetNode n, int label);

	void drawBoundary(
			const CragVolumes&           volumes,
			Crag::Node                   n,
			vigra::MultiArray<3, float>& components,
			float                        value);

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

    std::vector<LinearConstraint> _allTreePathConstraints;
};

#endif // CANDIDATE_MC_SOLVER_MULTI_CUT_H__

