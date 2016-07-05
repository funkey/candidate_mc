#include <boost/filesystem.hpp>
#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>
#include <solver/SolverFactory.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/box.hpp>
#include "ClosedSetSolver.h"

logger::LogChannel closedsetlog("closedsetlog", "[ClosedSetSolver] ");

ClosedSetSolver::ClosedSetSolver(const Crag& crag, const Parameters& parameters) :
	_crag(crag),
	_numNodes(0),
	_numEdges(0),
	_solver(0),
	_parameters(parameters) {

	_numNodes = _crag.nodes().size();
	_numEdges = _crag.edges().size();

	SolverFactory factory;
	_solver = factory.createLinearSolverBackend();

	prepareSolver();
	setVariables();
	if (!_parameters.noConstraints)
		setInitialConstraints();
}

ClosedSetSolver::~ClosedSetSolver() {

	if (_solver)
		delete _solver;
}

void
ClosedSetSolver::setCosts(const Costs& costs) {

	for (Crag::CragNode n : _crag.nodes())
		_objective.setCoefficient(
				nodeIdToVar(_crag.id(n)),
				costs.node[n]);

	for (Crag::CragEdge e : _crag.edges())
		_objective.setCoefficient(
				edgeIdToVar(_crag.id(e)),
				costs.edge[e]);
}

ClosedSetSolver::Status
ClosedSetSolver::solve(CragSolution& solution) {

	_solver->setObjective(_objective);

	for (unsigned int i = 0; i < _parameters.numIterations; i++) {

		LOG_USER(closedsetlog)
				<< "------------------------ iteration "
				<< i << std::endl;

		findMinClosedSet(solution);

		if (!findViolatedConstraints(solution)) {

			LOG_USER(closedsetlog)
					<< "optimal solution with value "
					<< _solution.getValue() << " found"
					<< std::endl;

			int numSelected = 0;
			int numMerged = 0;
			double avgDepth = 0;
			for (Crag::CragNode n : _crag.nodes())
				if (solution.selected(n)) {

					numSelected++;
					avgDepth += _crag.getLevel(n);
				}
			avgDepth /= numSelected;

			for (Crag::CragEdge e : _crag.edges())
				if (solution.selected(e))
					numMerged++;

			LOG_USER(closedsetlog)
					<< numSelected << " candidates selected, "
					<< numMerged << " adjacent candidates merged"
					<< std::endl;
			LOG_USER(closedsetlog)
					<< "average depth of selected candidates is " << avgDepth
					<< std::endl;

			return SolutionFound;
		}
	}

	LOG_USER(closedsetlog) << "maximum number of iterations reached" << std::endl;
	return MaxIterationsReached;
}

void
ClosedSetSolver::prepareSolver() {

	LOG_DEBUG(closedsetlog) << "preparing solver..." << std::endl;

	// one binary indicator per node and edge
	_objective.resize(_numNodes + _numEdges);
	_objective.setSense(_parameters.minimize ? Minimize : Maximize);

	_solver->initialize(_numNodes + _numEdges, Binary);
}

void
ClosedSetSolver::setVariables() {

	LOG_DEBUG(closedsetlog) << "setting variables..." << std::endl;

	// node ids match 1:1 with variable numbers
	unsigned int nextVar = _numNodes;

	// adjacency edges are mapped in order of appearance
	for (Crag::CragEdge e : _crag.edges()) {

		_edgeIdToVarMap[_crag.id(e)] = nextVar;
		nextVar++;
	}
}

void
ClosedSetSolver::setInitialConstraints() {

	LOG_DEBUG(closedsetlog) << "setting initial constraints..." << std::endl;

	// node-node constraints: every selected nod implies selection of its 
	// children

	int numNodeNodeConstraints = 0;

	for (Crag::CragNode n : _crag.nodes())
		for (Crag::CragArc a : _crag.inArcs(n)) {

			int p = nodeIdToVar(_crag.id(n));
			int c = nodeIdToVar(_crag.id(a.source()));

			LinearConstraint constraint;
			constraint.setCoefficient(p,  1);
			constraint.setCoefficient(c, -1);
			constraint.setRelation(LessEqual);
			constraint.setValue(0);
			_constraints.add(constraint);

			numNodeNodeConstraints++;
		}

	LOG_USER(closedsetlog)
			<< "added " << numNodeNodeConstraints
			<< " node-node constraints" << std::endl;

	// edge-node constraints: every selected edge implies selection of its 
	// incident nodes

	int numEdgeNodeConstraints = 0;

	for (Crag::CragEdge e : _crag.edges())
		for (Crag::CragNode n : { e.u(), e.v() }) {

			int p = edgeIdToVar(_crag.id(e));
			int c = nodeIdToVar(_crag.id(n));

			LinearConstraint constraint;
			constraint.setCoefficient(p,  1);
			constraint.setCoefficient(c, -1);
			constraint.setRelation(LessEqual);
			constraint.setValue(0);
			_constraints.add(constraint);

			numEdgeNodeConstraints++;
		}

	LOG_USER(closedsetlog)
			<< "added " << numEdgeNodeConstraints
			<< " edge-node constraints" << std::endl;

	// node-edge constraints: every selected node implies selection of adjacency 
	// edges between descendant nodes

	int numNodeEdgeConstraints = 0;
	for (Crag::CragNode n : _crag.nodes())
		for (Crag::CragArc a : _crag.inArcs(n))
			for (Crag::CragArc b : _crag.inArcs(n))
				if (_crag.id(a) < _crag.id(b))
					for (Crag::CragEdge e : _crag.descendantEdges(a.source(), b.source())) {

						int p = nodeIdToVar(_crag.id(n));
						int c = edgeIdToVar(_crag.id(e));

						LinearConstraint constraint;
						constraint.setCoefficient(p,  1);
						constraint.setCoefficient(c, -1);
						constraint.setRelation(LessEqual);
						constraint.setValue(0);
						_constraints.add(constraint);

						numNodeEdgeConstraints++;
					}

	LOG_USER(closedsetlog)
			<< "added " << numNodeEdgeConstraints
			<< " node-edge constraints" << std::endl;

	// edge-edge constraints: every selected edge implies selection of 
	// descendent edges

	int numEdgeEdgeConstraints = 0;

	for (Crag::CragEdge e : _crag.edges())
		for (Crag::CragEdge f : _crag.descendantEdges(e)) {

			int p = edgeIdToVar(_crag.id(e));
			int c = edgeIdToVar(_crag.id(f));

			LinearConstraint constraint;
			constraint.setCoefficient(p,  1);
			constraint.setCoefficient(c, -1);
			constraint.setRelation(LessEqual);
			constraint.setValue(0);
			_constraints.add(constraint);

			numEdgeEdgeConstraints++;
		}

	LOG_USER(closedsetlog)
			<< "added " << numEdgeEdgeConstraints
			<< " edge-edge constraints" << std::endl;
}

void
ClosedSetSolver::findMinClosedSet(CragSolution& solution) {

	LOG_USER(closedsetlog) << "searching for min closed set..." << std::endl;

	// re-set constraints to inform solver about potential changes
	_solver->setConstraints(_constraints);
	std::string msg;
	if (!_solver->solve(_solution, msg)) {

		LOG_ERROR(closedsetlog) << "solver did not find optimal solution: " << msg << std::endl;

	} else {

		LOG_DEBUG(closedsetlog) << "solver returned solution with message: " << msg << std::endl;
	}

	// get selected candidates
	for (Crag::CragNode n : _crag.nodes()) {

		LOG_ALL(closedsetlog) << _crag.id(n) << std::endl;
		LOG_ALL(closedsetlog) << nodeIdToVar(_crag.id(n)) << std::endl;
		LOG_ALL(closedsetlog) << _solution[nodeIdToVar(_crag.id(n))] << std::endl;

		solution.setSelected(n, (_solution[nodeIdToVar(_crag.id(n))] > 0.5));

		LOG_ALL(closedsetlog)
				<< _crag.id(n) << ": "
				<< solution.selected(n)
				<< std::endl;
	}

	// get merged edges
	for (Crag::CragEdge e : _crag.edges()) {

		solution.setSelected(e, (_solution[edgeIdToVar(_crag.id(e))] > 0.5));

		LOG_ALL(closedsetlog)
				<< "(" << _crag.id(_crag.u(e))
				<< "," << _crag.id(_crag.v(e))
				<< "): " << solution.selected(e)
				<< std::endl;
	}
}

bool
ClosedSetSolver::findViolatedConstraints(CragSolution& solution) {

	int constraintsAdded = 0;

	if (_parameters.noConstraints)
		return false;

	// Cut graph contains all nodes and all selected leaf edges.
	typedef lemon::ListGraph Cut;
	Cut cutGraph;
	for (unsigned int i = 0; i < _numNodes; i++)
		cutGraph.addNode();

	for (Crag::CragEdge e : _crag.edges())
		if (solution.selected(e) && _crag.isLeafEdge(e))
			cutGraph.addEdge(_crag.u(e), _crag.v(e));

	// for each not selected edge with nodes in the same connected component, 
	// find the shortest path along connected nodes connecting them
	for (Crag::CragEdge e : _crag.edges()) {

		// consider only leaf edges
		if (!_crag.isLeafEdge(e))
			continue;

		// only not selected edges
		if (solution.selected(e))
			continue;

		Crag::CragNode s = e.u();
		Crag::CragNode t = e.v();

		// only with incident nodes of same component
		if (solution.label(s) != solution.label(t) || !solution.selected(s))
			continue;

		LOG_ALL(closedsetlog)
				<< "nodes " << _crag.id(s)
				<< " and " << _crag.id(t)
				<< " (edge " << edgeIdToVar(_crag.id(e))
				<< ") are cut, but in same component"
				<< std::endl;

		// setup Dijkstra
		One one;
		lemon::Dijkstra<Cut, One> dijkstra(cutGraph, one);

		// e = (s, t) was not selected -> there should be no path connecting s 
		// and t, but there is (at least) one, let's find it
		if (!dijkstra.run(s, t))
			LOG_ERROR(closedsetlog)
					<< "dijkstra could not find a path!"
					<< std::endl;

		LinearConstraint cycleConstraint;

		int lenPath = 0;

		// walk along the path between u and v
		Crag::CragNode cur = t;
		LOG_ALL(closedsetlog)
				<< "nodes " << _crag.id(s)
				<< " and " << _crag.id(t)
				<< " (edge " << edgeIdToVar(_crag.id(e)) << ")"
				<< " are connected via path ";
		while (cur != s) {

			LOG_ALL(closedsetlog)
					<< _crag.id(cur)
					<< " ";

			Crag::Node pre(dijkstra.predNode(cur));

			// here we have to iterate over all adjacent edges in order to 
			// find (cur, pre) in CRAG, since there is no 1:1 mapping 
			// between edges in cutGraph and _crag
			Crag::CragIncEdgeIterator pathEdge = _crag.adjEdges(cur).begin();
			for (; pathEdge != _crag.adjEdges(cur).end(); ++pathEdge)
				if ((*pathEdge).opposite(cur) == pre)
					break;

			if (pathEdge == _crag.adjEdges(cur).end())
				UTIL_THROW_EXCEPTION(
						Exception,
						"could not find path edge in CRAG");

			if (!solution.selected(*pathEdge))
				LOG_ERROR(closedsetlog)
						<< "edge " << edgeIdToVar(_crag.id(*pathEdge))
						<< " is not selected, but found by dijkstra"
						<< std::endl;
				//UTIL_THROW_EXCEPTION(
						//Exception,
						//"edge " << edgeIdToVar(_crag.id(pathEdge)) << " is not selected, but found by dijkstra");

			cycleConstraint.setCoefficient(
					edgeIdToVar(_crag.id(*pathEdge)),
					1.0);
			LOG_ALL(closedsetlog)
					<< "(edge " << edgeIdToVar(_crag.id(*pathEdge)) << ") ";

			lenPath++;
			cur = pre;
		}
		LOG_ALL(closedsetlog) << _crag.id(s) << std::endl;

		cycleConstraint.setCoefficient(
				edgeIdToVar(_crag.id(e)),
				-1.0);
		cycleConstraint.setRelation(LessEqual);
		cycleConstraint.setValue(lenPath - 1);

		LOG_ALL(closedsetlog) << cycleConstraint << std::endl;

		_constraints.add(cycleConstraint);

		constraintsAdded++;

		if (_parameters.maxConstraintsPerIteration > 0 &&
			constraintsAdded >= _parameters.maxConstraintsPerIteration)
			break;
	}

	LOG_USER(closedsetlog)
			<< "added " << constraintsAdded
			<< " cycle constraints" << std::endl;

	return constraintsAdded > 0;
}

