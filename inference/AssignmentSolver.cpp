#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>
#include <solver/SolverFactory.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/box.hpp>
#include <util/assert.h>
#include <util/helpers.hpp>
#include "AssignmentSolver.h"

#include <vigra/multi_impex.hxx>
#include <vigra/functorexpression.hxx>

logger::LogChannel assignmentlog("assignmentlog", "[AssignmentSolver] ");

AssignmentSolver::AssignmentSolver(
		const Crag& crag,
		const CragVolumes& volumes,
		const Parameters& parameters) :
	_crag(crag),
	_volumes(volumes),
	_numNodes(crag.nodes().size()),
	_numEdges(crag.edges().size()),
	_parameters(parameters) {

	SolverFactory factory;
	_solver = std::unique_ptr<LinearSolverBackend>(factory.createLinearSolverBackend());

	checkCrag();
	prepareSolver();
	setVariables();
	setConstraints();
}

void
AssignmentSolver::checkCrag() {

	for (Crag::CragNode n : _crag.nodes())
		if (_crag.type(n) == Crag::VolumeNode)
			UTIL_THROW_EXCEPTION(
					UsageError,
					"AssignmentSolver can not be used on CRAGs with nodes of type VolumeNode. "
					"Your CRAG should only contain SliceNodes, AssignmentNodes, and NoAssignmentNodes.");

	// this solver assumes a CRAG with only AssignmentEdges, NoAssignmentEdges.

	for (Crag::CragEdge e : _crag.edges())
		if (_crag.type(e) == Crag::AdjacencyEdge)
			UTIL_THROW_EXCEPTION(
					UsageError,
					"AssignmentSolver can not be used on CRAGs with edges of type AdjacencyEdge. "
					"Your CRAG should only contain AssignmentEdges and NoAssignmentEdges.");
}

void
AssignmentSolver::setCosts(const Costs& costs) {

	for (Crag::CragNode n : _crag.nodes())
		_objective.setCoefficient(
				nodeIdToVar(_crag.id(n)),
				costs.node[n]);

	// only no-assignment edges have a cost
	for (Crag::CragEdge e : _crag.edges())
		if (_crag.type(e) == Crag::NoAssignmentEdge)
			_objective.setCoefficient(
					edgeIdToVar(_crag.id(e)),
					costs.edge[e]);
}

AssignmentSolver::Status
AssignmentSolver::solve(CragSolution& solution) {

	_solver->setObjective(_objective);

	findAssignments(solution);

	return SolutionFound;
}

void
AssignmentSolver::prepareSolver() {

	LOG_DEBUG(assignmentlog) << "preparing solver..." << std::endl;

	// one binary indicator per node and edge
	_objective.resize(_numNodes + _numEdges);
	_objective.setSense(_parameters.minimize ? Minimize : Maximize);

	_solver->initialize(_numNodes + _numEdges, Binary);
}

void
AssignmentSolver::setVariables() {

	LOG_DEBUG(assignmentlog) << "setting variables..." << std::endl;

	// node ids match 1:1 with variable numbers
	unsigned int nextVar = _numNodes;

	// edges are mapped in order of appearance
	for (Crag::CragEdge e : _crag.edges()) {

		_edgeIdToVarMap[_crag.id(e)] = nextVar;
		nextVar++;
	}
}

void
AssignmentSolver::setConstraints() {

	LOG_DEBUG(assignmentlog) << "setting constraints..." << std::endl;

	// tree-path constraints: from all nodes along a path in the CRAG subset 
	// tree, at most one can be chosen

	int numTreePathConstraints = 0;

	// for each root (excluding assignment nodes)
	for (Crag::CragNode n : _crag.nodes()) {

		// count the parents (AssignmentNodes are not parents)
		int numParents = 0;
		for (Crag::CragArc a : _crag.outArcs(n))
			if (_crag.type(a.target()) != Crag::AssignmentNode)
				numParents++;

		if (numParents > 0 || _crag.type(n) == Crag::AssignmentNode)
			continue;

		std::vector<int> pathIds;
		int added = collectTreePathConstraints(n, pathIds);

		numTreePathConstraints += added;
	}

	LOG_USER(assignmentlog)
			<< "added " << numTreePathConstraints
			<< " tree-path constraints" << std::endl;

	// explanation constraints: for each selected slice, exactly one assignment 
	// edge has to be selected to +z and -z

	int numExplanationConstraints = 0;

	// for each slice node
	for (Crag::CragNode n : _crag.nodes()) {

		if (_crag.type(n) != Crag::SliceNode)
			continue;

		for (int direction : { +1, -1 }) {

			LinearConstraint explanationConstraint;

			int numIncEdges = 0;

			// for each adjacent edge in direction
			for (Crag::CragEdge e : _crag.adjEdges(n)) {

				Crag::CragNode other = e.opposite(n);

				// z offset from n to other
				float zDiff =
						_volumes[other]->getBoundingBox().center().z() -
						_volumes[n]->getBoundingBox().center().z();

				LOG_ALL(assignmentlog) << "bb n    : " << _volumes[n]->getBoundingBox() << std::endl;
				LOG_ALL(assignmentlog) << "bb other: " << _volumes[other]->getBoundingBox() << std::endl;
				LOG_ALL(assignmentlog) << _crag.id(n) << " vs. " << _crag.id(other) << " dir " << direction << " zdiff " << zDiff << std::endl;

				// if not in the right direction, skip this edge
				if (zDiff*direction < 0)
					continue;

				LOG_ALL(assignmentlog) << "direction lines up" << std::endl;

				// sum of edges in this direction...
				explanationConstraint.setCoefficient(
						edgeIdToVar(_crag.id(e)),
						1.0);
				numIncEdges++;
			}

			// (there should be at least the no-assignment edge)
			UTIL_ASSERT_REL(numIncEdges, >, 0);

			// ...minus candidate...
			explanationConstraint.setCoefficient(
					nodeIdToVar(_crag.id(n)),
					-1);

			// ...should be exactly zero
			explanationConstraint.setRelation(Equal);
			explanationConstraint.setValue(0.0);

			LOG_ALL(assignmentlog) << explanationConstraint << std::endl;

			_constraints.add(explanationConstraint);
			numExplanationConstraints++;
		}
	}

	LOG_USER(assignmentlog)
			<< "added " << numExplanationConstraints
			<< " rejection constraints" << std::endl;

	// assignment constraints: all adjacency edges of a selected assignment node 
	// have to be chosen

	int numAssignmentConstraints = 0;
	for (Crag::CragNode n : _crag.nodes()) {

		if (_crag.type(n) != Crag::AssignmentNode)
			continue;

		for (Crag::CragEdge e : _crag.adjEdges(n)) {

			UTIL_ASSERT(_crag.type(e) == Crag::AssignmentEdge);

			LinearConstraint identityConstraint;
			identityConstraint.setCoefficient(nodeIdToVar(_crag.id(n)),  1);
			identityConstraint.setCoefficient(edgeIdToVar(_crag.id(e)), -1);
			identityConstraint.setRelation(Equal);
			identityConstraint.setValue(0);

			LOG_ALL(assignmentlog) << identityConstraint << std::endl;

			_constraints.add(identityConstraint);
			numAssignmentConstraints++;
		}
	}

	LOG_USER(assignmentlog)
			<< "added " << numAssignmentConstraints
			<< " assignment constraints" << std::endl;

	_solver->setConstraints(_constraints);
}

int
AssignmentSolver::collectTreePathConstraints(Crag::CragNode n, std::vector<int>& pathIds) {

	int numConstraintsAdded = 0;

	pathIds.push_back(_crag.id(n));

	int numChildren = 0;
	for (Crag::CragArc c : _crag.inArcs(n)) {

		numConstraintsAdded +=
				collectTreePathConstraints(
						c.source(),
						pathIds);
		numChildren++;
	}

	if (numChildren == 0 && pathIds.size() > 1) {

		LOG_ALL(assignmentlog) << "adding tree-path constraints for " << pathIds << std::endl;

		LinearConstraint treePathConstraint;

		for (int id : pathIds)
			treePathConstraint.setCoefficient(
					nodeIdToVar(id),
					1.0);

		if (_parameters.forceExplanation)
			treePathConstraint.setRelation(Equal);
		else
			treePathConstraint.setRelation(LessEqual);

		treePathConstraint.setValue(1.0);

		_constraints.add(treePathConstraint);
		numConstraintsAdded++;
	}

	pathIds.pop_back();

	return numConstraintsAdded;
}

void
AssignmentSolver::findAssignments(CragSolution& solution) {

	LOG_USER(assignmentlog) << "searching for optimal assignments..." << std::endl;

	std::string msg;
	if (!_solver->solve(_solution, msg))
		LOG_ERROR(assignmentlog) << "solver did not find optimal solution: " << msg << std::endl;

	// get selected candidates
	for (Crag::CragNode n : _crag.nodes()) {

		solution.setSelected(n, (_solution[nodeIdToVar(_crag.id(n))] > 0.5));

		LOG_ALL(assignmentlog)
				<< _crag.id(n) << ": "
				<< solution.selected(n)
				<< std::endl;
	}

	// get merged edges
	for (Crag::CragEdge e : _crag.edges()) {

		solution.setSelected(e, (_solution[edgeIdToVar(_crag.id(e))] > 0.5));

		LOG_ALL(assignmentlog)
				<< "(" << _crag.id(_crag.u(e))
				<< "," << _crag.id(_crag.v(e))
				<< "): " << solution.selected(e)
				<< std::endl;
	}
}

