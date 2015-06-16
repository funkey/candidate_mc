#ifndef CANDIDATE_MC_SOLVER_MULTI_CUT_H__
#define CANDIDATE_MC_SOLVER_MULTI_CUT_H__

#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <crag/Crag.h>
#include <solver/LinearSolver.h>
#include "Costs.h"

class MultiCut {

public:

	struct Parameters {

		Parameters() :
			forceExplanation(false),
			numIterations(100),
			maxConstraintsPerIteration(0),
			minimize(true) {}

		/**
		 * If true, force exactly one region to be chosen for each root-to-leaf 
		 * path in the subset tree of the CRAG. This implies that there will be 
		 * no background region.
		 */
		bool forceExplanation;

		/**
		 * The maximal number of iterations to solve the multi-cut problem.
		 */
		int numIterations;

		/**
		 * The maximal number of cycle constraints to add per iteration.
		 */
		int maxConstraintsPerIteration;

		bool minimize;
	};

	enum Status {

		SolutionFound,

		MaxIterationsReached
	};

	MultiCut(const Crag& crag, const Parameters& parameters = Parameters());

	/**
	 * Set the costs (or reward, if negative) of accepting a node or an edge.
	 */
	void setCosts(const Costs& costs);

	Status solve(unsigned int numIterations = 0);

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
	double getValue() { return _solution->getValue(); }

	/**
	 * Store the solution as label image in the given image file.
	 */
	void storeSolution(const std::string& filename, bool drawBoundary = false);

private:

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
			Crag::Node                   n,
			vigra::MultiArray<3, float>& components,
			float                        value);

	ExplicitVolume<bool> getVolume(Crag::SubsetNode n);

	inline unsigned int nodeIdToVar(int nodeId) { return nodeId; }
	inline unsigned int edgeIdToVar(int edgeId) { return _edgeIdToVarMap[edgeId]; }

	const Crag& _crag;

	Crag::EdgeMap<bool> _merged;
	Crag::NodeMap<bool> _selected;
	Crag::NodeMap<int>  _labels;

	unsigned int _numNodes, _numEdges;

	std::map<int, unsigned int> _edgeIdToVarMap;

	pipeline::Value<LinearObjective>        _objective;
	pipeline::Value<LinearConstraints>      _constraints;
	pipeline::Value<LinearSolverParameters> _solverParameters;
	pipeline::Process<LinearSolver>         _solver;
	pipeline::Value<Solution>               _solution;

	Parameters _parameters;
};

#endif // CANDIDATE_MC_SOLVER_MULTI_CUT_H__

