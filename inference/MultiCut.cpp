#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>
#include <util/Logger.h>
#include "MultiCut.h"

logger::LogChannel multicutlog("multicutlog", "[MultiCut] ");

MultiCut::MultiCut(const Crag& crag, const Parameters& parameters) :
	_crag(crag),
	_merged(crag),
	_components(crag),
	_numNodes(0),
	_numEdges(0),
	_parameters(parameters) {

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		_numNodes++;
	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e)
		_numEdges++;

	prepareSolver();
	setVariables();
	setInitialConstraints();
}

void
MultiCut::setNodeCosts(const Crag::NodeMap<double>& nodeCosts) {

	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
		_objective->setCoefficient(
				nodeIdToVar(_crag.id(n)),
				nodeCosts[n]);
}

void
MultiCut::setEdgeCosts(const Crag::EdgeMap<double>& edgeCosts) {

	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e)
		_objective->setCoefficient(
				edgeIdToVar(_crag.id(e)),
				edgeCosts[e]);
}

MultiCut::Status
MultiCut::solve(unsigned int numIterations) {

	if (numIterations == 0)
		numIterations = _parameters.numIterations;

	_solver->setInput("objective", _objective);

	for (unsigned int i = 0; i < numIterations; i++) {

		LOG_USER(multicutlog)
				<< "------------------------ iteration "
				<< i << std::endl;

		findCut();

		if (!findViolatedConstraints()) {

			LOG_USER(multicutlog) << "optimal solution found" << std::endl;
			return SolutionFound;
		}
	}

	LOG_USER(multicutlog) << "maximum number of iterations reached" << std::endl;
	return MaxIterationsReached;
}

void
MultiCut::prepareSolver() {

	LOG_DEBUG(multicutlog) << "preparing solver..." << std::endl;

	// one binary indicator per node and edge
	_objective->resize(_numNodes + _numEdges);
	_solverParameters->setVariableType(Binary);

	_solver->setInput("parameters", _solverParameters);

	_solution = _solver->getOutput("solution");
}

void
MultiCut::setVariables() {

	LOG_DEBUG(multicutlog) << "setting variables..." << std::endl;

	// node ids match 1:1 with variable numbers
	unsigned int nextVar = _numNodes;

	// adjacency edges are mapped in order of appearance
	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e) {

		_edgeIdToVarMap[_crag.id(e)] = nextVar;
		nextVar++;
	}
}

void
MultiCut::setInitialConstraints() {

	LOG_DEBUG(multicutlog) << "setting initial constraints..." << std::endl;

	// tree-path constraints: from all nodes along a path in the CRAG subset 
	// tree, at most one can be chosen

	int numTreePathConstraints = 0;

	// for each root
	for (Crag::SubsetType::NodeIt n(_crag); n != lemon::INVALID; ++n) {

		int numParents = 0;
		for (Crag::SubsetOutArcIt p(_crag, n); p != lemon::INVALID; ++p)
			numParents++;

		if (numParents == 0) {

			std::vector<int> pathIds;
			int added = collectTreePathConstraints(n, pathIds);

			numTreePathConstraints += added;
		}
	}

	LOG_USER(multicutlog)
			<< "added " << numTreePathConstraints
			<< " tree-path constraints" << std::endl;

	// rejection constraints: none of the adjacency edges of a rejected node are 
	// allowed to be chosen

	int numRejectionConstraints = 0;

	// for each node
	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

		LinearConstraint rejectionConstraint;

		int numIncEdges = 0;

		// for each adjacent edge
		for (Crag::IncEdgeIt e(_crag, n); e != lemon::INVALID; ++e) {

			rejectionConstraint.setCoefficient(
					edgeIdToVar(_crag.id(e)),
					1.0);
			numIncEdges++;
		}

		rejectionConstraint.setCoefficient(
				nodeIdToVar(_crag.id(n)),
				-numIncEdges);

		rejectionConstraint.setRelation(LessEqual);
		rejectionConstraint.setValue(0.0);

		_constraints->add(rejectionConstraint);
		numRejectionConstraints++;
	}

	LOG_USER(multicutlog)
			<< "added " << numRejectionConstraints
			<< " rejection constraints" << std::endl;
}

int
MultiCut::collectTreePathConstraints(Crag::SubsetNode n, std::vector<int>& pathIds) {

	int numConstraintsAdded = 0;

	pathIds.push_back(_crag.getSubsetGraph().id(n));

	int numChildren = 0;
	for (Crag::SubsetInArcIt c(_crag, n); c != lemon::INVALID; ++c) {

		numConstraintsAdded +=
				collectTreePathConstraints(
						_crag.getSubsetGraph().oppositeNode(n, c),
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

		_constraints->add(treePathConstraint);
		numConstraintsAdded++;
	}

	pathIds.pop_back();

	return numConstraintsAdded;
}

void
MultiCut::findCut() {

	// re-set constraints to inform solver about potential changes
	_solver->setInput("linear constraints", _constraints);

	LOG_USER(multicutlog) << "searching for optimal cut..." << std::endl;

	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e) {

		_merged[e] = ((*_solution)[edgeIdToVar(_crag.id(e))] > 0.5);

		LOG_ALL(multicutlog)
				<< "(" << _crag.id(_crag.u(e))
				<< "," << _crag.id(_crag.v(e))
				<< "): " << _merged[e]
				<< std::endl;
	}
}

bool
MultiCut::findViolatedConstraints() {

	int constraintsAdded = 0;

	// Given the large number of adjacency edges, and that only a small subset 
	// of them gets selected, it might be more efficient to create a new graph, 
	// here, consisting only of the selected adjacency edges.

	typedef lemon::ListGraph Cut;
	Cut cutGraph;
	for (unsigned int i = 0; i < _numNodes; i++)
		cutGraph.addNode();

	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e)
		if (_merged[e])
			cutGraph.addEdge(_crag.u(e), _crag.v(e));

	// find connected components in cut graph
	lemon::connectedComponents(cutGraph, _components);

	// label rejected nodes with -1
	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
		if ((*_solution)[nodeIdToVar(_crag.id(n))] < 0.5)
			_components[n] = -1;

	// propagate node labels to subsets
	for (Crag::SubsetNodeIt n(_crag); n != lemon::INVALID; ++n) {

		int numParents = 0;
		for (Crag::SubsetOutArcIt e(_crag, n); e != lemon::INVALID; ++e)
			numParents++;

		if (numParents == 0)
			propagateLabel(n, -1);
	}

	// for each not selected edge with nodes in the same connected component, 
	// find the shortest path along connected nodes connecting them
	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e) {

		// not selected
		if (_merged[e])
			continue;

		Crag::Node s = _crag.u(e);
		Crag::Node t = _crag.v(e);

		// in same component
		if (_components[s] != _components[t] || _components[s] == -1)
			continue;

		LOG_DEBUG(multicutlog)
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

		LOG_DEBUG(multicutlog)
				<< "nodes " << _crag.id(s)
				<< " and " << _crag.id(t)
				<< " are in same component "
				<< _components[_crag.v(e)]
				<< std::endl;

		LinearConstraint cycleConstraint;

		int lenPath = 0;

		// walk along the path between u and v
		Crag::Node cur = t;
		LOG_DEBUG(multicutlog)
				<< "nodes " << _crag.id(s)
				<< " and " << _crag.id(t)
				<< " (edge " << edgeIdToVar(_crag.id(e)) << ")"
				<< " are connected via path ";
		while (cur != s) {

			LOG_DEBUG(multicutlog)
					<< _crag.id(cur)
					<< " ";

			Crag::Node pre = dijkstra.predNode(cur);

			// here we have to iterate over all adjacent edges in order to 
			// find (cur, pre) in CRAG, since there is no 1:1 mapping 
			// between edges in cutGraph and _crag
			Crag::IncEdgeIt pathEdge(_crag, cur);
			for (; pathEdge!= lemon::INVALID; ++pathEdge)
				if (_crag.getAdjacencyGraph().oppositeNode(cur, pathEdge) == pre)
					break;

			if (pathEdge == lemon::INVALID)
				UTIL_THROW_EXCEPTION(
						Exception,
						"could not find path edge in CRAG");

			if (!_merged[pathEdge])
				LOG_ERROR(multicutlog)
						<< "edge " << edgeIdToVar(_crag.id(pathEdge))
						<< " is not selected, but found by dijkstra"
						<< std::endl;
				//UTIL_THROW_EXCEPTION(
						//Exception,
						//"edge " << edgeIdToVar(_crag.id(pathEdge)) << " is not selected, but found by dijkstra");

			cycleConstraint.setCoefficient(
					edgeIdToVar(_crag.id(pathEdge)),
					1.0);
			LOG_DEBUG(multicutlog)
					<< "(edge " << edgeIdToVar(_crag.id(pathEdge)) << ") ";

			lenPath++;
			cur = pre;
		}
		LOG_DEBUG(multicutlog) << _crag.id(s) << std::endl;

		cycleConstraint.setCoefficient(
				edgeIdToVar(_crag.id(e)),
				-1.0);
		cycleConstraint.setRelation(LessEqual);
		cycleConstraint.setValue(lenPath - 1);

		LOG_DEBUG(multicutlog) << cycleConstraint << std::endl;

		_constraints->add(cycleConstraint);

		constraintsAdded++;

		if (_parameters.maxConstraintsPerIteration > 0 &&
			constraintsAdded >= _parameters.maxConstraintsPerIteration)
			break;
	}

	LOG_USER(multicutlog)
			<< "added " << constraintsAdded
			<< " cycle constraints" << std::endl;

	return (constraintsAdded > 0);
}

void
MultiCut::propagateLabel(Crag::SubsetNode n, int label) {

	if (label == -1)
		label = _components[_crag.toRag(n)];

	for (Crag::SubsetInArcIt e(_crag, n); e != lemon::INVALID; ++e)
		propagateLabel(_crag.getSubsetGraph().oppositeNode(n, e), label);
}
