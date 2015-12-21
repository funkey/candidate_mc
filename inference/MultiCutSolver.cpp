#include <boost/filesystem.hpp>
#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>
#include <solver/SolverFactory.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/box.hpp>
#include "MultiCutSolver.h"

logger::LogChannel multicutlog("multicutlog", "[MultiCutSolver] ");

util::ProgramOption optionForceParentCandidate(
		util::_long_name        = "forceParentCandidate",
		util::_description_text = "Disallow merging of children into a shape that resembles their parent. "
		                          "In this case, take the parent instead.");

util::ProgramOption optionLazyTreePathConstraints(
        util::_long_name        = "lazyTreePathConstraints",
        util::_description_text = "Only add violated tree path constraints",
        util::_default_value    = false);

MultiCutSolver::MultiCutSolver(const Crag& crag, const Parameters& parameters) :
	_crag(crag),
	_numNodes(0),
	_numEdges(0),
	_solver(0),
	_parameters(parameters),
	_labels(crag) {

	_numNodes = _crag.nodes().size();
	_numEdges = _crag.edges().size();

	SolverFactory factory;
	_solver = factory.createLinearSolverBackend();

	prepareSolver();
	setVariables();
	if (!_parameters.noConstraints)
		setInitialConstraints();
}

MultiCutSolver::~MultiCutSolver() {

	if (_solver)
		delete _solver;
}

void
MultiCutSolver::setCosts(const Costs& costs) {

	for (Crag::CragNode n : _crag.nodes())
		_objective.setCoefficient(
				nodeIdToVar(_crag.id(n)),
				costs.node[n]);

	for (Crag::CragEdge e : _crag.edges())
		_objective.setCoefficient(
				edgeIdToVar(_crag.id(e)),
				costs.edge[e]);
}

MultiCutSolver::Status
MultiCutSolver::solve(CragSolution& solution) {

	_solver->setObjective(_objective);

	for (unsigned int i = 0; i < _parameters.numIterations; i++) {

		LOG_USER(multicutlog)
				<< "------------------------ iteration "
				<< i << std::endl;

		findCut(solution);

		if (!findViolatedConstraints(solution)) {

			LOG_USER(multicutlog)
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

			LOG_USER(multicutlog)
					<< numSelected << " candidates selected, "
					<< numMerged << " adjacent candidates merged"
					<< std::endl;
			LOG_USER(multicutlog)
					<< "average depth of selected candidates is " << avgDepth
					<< std::endl;

			return SolutionFound;
		}
	}

	LOG_USER(multicutlog) << "maximum number of iterations reached" << std::endl;
	return MaxIterationsReached;
}

void
MultiCutSolver::prepareSolver() {

	LOG_DEBUG(multicutlog) << "preparing solver..." << std::endl;

	// one binary indicator per node and edge
	_objective.resize(_numNodes + _numEdges);
	_objective.setSense(_parameters.minimize ? Minimize : Maximize);

	_solver->initialize(_numNodes + _numEdges, Binary);
}

void
MultiCutSolver::setVariables() {

	LOG_DEBUG(multicutlog) << "setting variables..." << std::endl;

	// node ids match 1:1 with variable numbers
	unsigned int nextVar = _numNodes;

	// adjacency edges are mapped in order of appearance
	for (Crag::CragEdge e : _crag.edges()) {

		_edgeIdToVarMap[_crag.id(e)] = nextVar;
		nextVar++;
	}
}

void
MultiCutSolver::setInitialConstraints() {

	LOG_DEBUG(multicutlog) << "setting initial constraints..." << std::endl;

	// tree-path constraints: from all nodes along a path in the CRAG subset 
	// tree, at most one can be chosen

	int numTreePathConstraints = 0;

	// for each root
	for (Crag::CragNode n : _crag.nodes())
		if (_crag.isRootNode(n)) {

			std::vector<int> pathIds;
			int added = collectTreePathConstraints(n, pathIds);

			numTreePathConstraints += added;
		}

	LOG_USER(multicutlog)
			<< "added " << numTreePathConstraints
			<< " tree-path constraints" << std::endl;

	// rejection constraints: none of the adjacency edges of a rejected node are 
	// allowed to be chosen

	int numRejectionConstraints = 0;

	// for each node
	for (Crag::CragNode n : _crag.nodes()) {

		LinearConstraint rejectionConstraint;

		int numIncEdges = 0;

		// for each adjacent edge
		for (Crag::CragEdge e : _crag.adjEdges(n)) {

			rejectionConstraint.setCoefficient(
					edgeIdToVar(_crag.id(e)),
					1.0);
			numIncEdges++;
		}

		if (numIncEdges == 0)
			continue;

		rejectionConstraint.setCoefficient(
				nodeIdToVar(_crag.id(n)),
				-numIncEdges);

		rejectionConstraint.setRelation(LessEqual);
		rejectionConstraint.setValue(0.0);

		_constraints.add(rejectionConstraint);
		numRejectionConstraints++;
	}

	LOG_USER(multicutlog)
			<< "added " << numRejectionConstraints
			<< " rejection constraints" << std::endl;

	if (!optionForceParentCandidate)
		return;

	int numForceParentConstraints = 0;

	for (Crag::CragNode n : _crag.nodes()) {

		std::vector<Crag::CragEdge> childEdges;

		// collect all child adjacency edges

		// for each child of n
		for (Crag::CragArc childEdge : _crag.inArcs(n)) {

			Crag::CragNode child = childEdge.source();

			// for each adjacent neighbor of child
			for (Crag::CragEdge childAdjacencyEdge : _crag.adjEdges(child)) {

				Crag::CragNode neighbor = childAdjacencyEdge.opposite(child);

				// only unique pairs
				if (neighbor < child)
					continue;

				// does neighbor have a parent?
				if (_crag.isRootNode(neighbor))
					continue;

				// is the parent of neighbor n?
				Crag::CragArc parentArc = *_crag.outArcs(neighbor).begin();
				Crag::CragNode parentNeighbor = parentArc.target();
				if (parentNeighbor == n)
					childEdges.push_back(childAdjacencyEdge);
			}
		}

		if (childEdges.size() == 0)
			continue;

		LinearConstraint forceParentConstraint;

		// require that not all of them are turned on at the same time
		for (Crag::CragEdge e : childEdges) {

			int var = _edgeIdToVarMap[_crag.id(e)];
			forceParentConstraint.setCoefficient(var, 1.0);
		}

		forceParentConstraint.setRelation(LessEqual);
		forceParentConstraint.setValue(childEdges.size() - 1);

		_constraints.add(forceParentConstraint);
		numForceParentConstraints++;
	}

	LOG_USER(multicutlog)
			<< "added " << numForceParentConstraints
			<< " force parent constraints" << std::endl;
}

int
MultiCutSolver::collectTreePathConstraints(Crag::CragNode n, std::vector<int>& pathIds) {

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

	if (numChildren == 0) {

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

        if(optionLazyTreePathConstraints.as<bool>()){
            _allTreePathConstraints.push_back(treePathConstraint);
        }
        else{
            _constraints.add(treePathConstraint);
            numConstraintsAdded++;
        }
	}

	pathIds.pop_back();

	return numConstraintsAdded;
}

void
MultiCutSolver::findCut(CragSolution& solution) {

	// re-set constraints to inform solver about potential changes
	_solver->setConstraints(_constraints);
	std::string msg;
	if (!_solver->solve(_solution, msg))
		LOG_ERROR(multicutlog) << "solver did not find optimal solution: " << msg << std::endl;

	LOG_USER(multicutlog) << "searching for optimal cut..." << std::endl;

	// get selected candidates
	for (Crag::CragNode n : _crag.nodes()) {

		solution.setSelected(n, (_solution[nodeIdToVar(_crag.id(n))] > 0.5));

		LOG_ALL(multicutlog)
				<< _crag.id(n) << ": "
				<< solution.selected(n)
				<< std::endl;
	}

	// get merged edges
	for (Crag::CragEdge e : _crag.edges()) {

		solution.setSelected(e, (_solution[edgeIdToVar(_crag.id(e))] > 0.5));

		LOG_ALL(multicutlog)
				<< "(" << _crag.id(_crag.u(e))
				<< "," << _crag.id(_crag.v(e))
				<< "): " << solution.selected(e)
				<< std::endl;
	}
}

bool
MultiCutSolver::findViolatedConstraints(CragSolution& solution) {

    int treePathConstraintAdded =0;
    int constraintsAdded = 0;

	if (_parameters.noConstraints)
		return false;




	// Given the large number of adjacency edges, and that only a small subset 
	// of them gets selected, it might be more efficient to create a new graph, 
	// here, consisting only of the selected adjacency edges.

	typedef lemon::ListGraph Cut;
	Cut cutGraph;
	for (unsigned int i = 0; i < _numNodes; i++)
		cutGraph.addNode();

	for (Crag::CragEdge e : _crag.edges())
		if (solution.selected(e))
			cutGraph.addEdge(_crag.u(e), _crag.v(e));

	// find connected components in cut graph
	lemon::connectedComponents(cutGraph, _labels);

    if(optionLazyTreePathConstraints.as<bool>()){
        for(auto & c : _allTreePathConstraints){
            if(c.isViolated(_solution)){
                _constraints.add(c);
                ++treePathConstraintAdded;
            }
        }
    }

	// label rejected nodes with -1
	for (Crag::CragNode n : _crag.nodes())
		if (!solution.selected(n))
			_labels[n] = -1;

	// for each not selected edge with nodes in the same connected component, 
	// find the shortest path along connected nodes connecting them
	for (Crag::CragEdge e : _crag.edges()) {

		// not selected
		if (solution.selected(e))
			continue;

		Crag::CragNode s = e.u();
		Crag::CragNode t = e.v();

		// in same component
		if (_labels[s] != _labels[t] || !solution.selected(s))
			continue;

		LOG_ALL(multicutlog)
				<< "nodes " << _crag.id(s)
				<< " and " << _crag.id(t)
				<< " (edge " << edgeIdToVar(_crag.id(e))
				<< ") are cut, but in same component"
				<< std::endl;

		// setup Dijkstra
		One one;
		lemon::Dijkstra<Cut, One> dijkstra(cutGraph, one);

		// e = (s, t) was not selected -> there should be no path connecting s 
		// and t
		if (!dijkstra.run(s, t))
			LOG_ERROR(multicutlog)
					<< "dijkstra could not find a path!"
					<< std::endl;

		LOG_ALL(multicutlog)
				<< "nodes " << _crag.id(s)
				<< " and " << _crag.id(t)
				<< " are in same component "
				<< _labels[_crag.v(e)]
				<< std::endl;

		LinearConstraint cycleConstraint;

		int lenPath = 0;

		// walk along the path between u and v
		Crag::CragNode cur = t;
		LOG_ALL(multicutlog)
				<< "nodes " << _crag.id(s)
				<< " and " << _crag.id(t)
				<< " (edge " << edgeIdToVar(_crag.id(e)) << ")"
				<< " are connected via path ";
		while (cur != s) {

			LOG_ALL(multicutlog)
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
				LOG_ERROR(multicutlog)
						<< "edge " << edgeIdToVar(_crag.id(*pathEdge))
						<< " is not selected, but found by dijkstra"
						<< std::endl;
				//UTIL_THROW_EXCEPTION(
						//Exception,
						//"edge " << edgeIdToVar(_crag.id(pathEdge)) << " is not selected, but found by dijkstra");

			cycleConstraint.setCoefficient(
					edgeIdToVar(_crag.id(*pathEdge)),
					1.0);
			LOG_ALL(multicutlog)
					<< "(edge " << edgeIdToVar(_crag.id(*pathEdge)) << ") ";

			lenPath++;
			cur = pre;
		}
		LOG_ALL(multicutlog) << _crag.id(s) << std::endl;

		cycleConstraint.setCoefficient(
				edgeIdToVar(_crag.id(e)),
				-1.0);
		cycleConstraint.setRelation(LessEqual);
		cycleConstraint.setValue(lenPath - 1);

		LOG_ALL(multicutlog) << cycleConstraint << std::endl;

		_constraints.add(cycleConstraint);

		constraintsAdded++;

		if (_parameters.maxConstraintsPerIteration > 0 &&
			constraintsAdded >= _parameters.maxConstraintsPerIteration)
			break;
	}

	LOG_USER(multicutlog)
			<< "added " << constraintsAdded
			<< " cycle constraints" << std::endl;

    if(optionLazyTreePathConstraints.as<bool>()){
    LOG_USER(multicutlog)
            << "added " << treePathConstraintAdded
            << " tree path  constraints" << std::endl;
    }

	// propagate node labels to subsets
	for (Crag::CragNode n : _crag.nodes())
		if (_crag.isRootNode(n))
			propagateLabel(n, -1);

	return (constraintsAdded + treePathConstraintAdded > 0);
}

void
MultiCutSolver::propagateLabel(Crag::CragNode n, int label) {

	if (label == -1)
		label = _labels[n];
	else
		_labels[n] = label;

	for (Crag::CragArc e : _crag.inArcs(n))
		propagateLabel(e.source(), label);
}

