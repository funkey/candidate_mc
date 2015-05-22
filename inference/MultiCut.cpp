#include <lemon/dijkstra.h>
#include <util/Logger.h>
#include "MultiCut.h"

logger::LogChannel multicutlog("multicutlog", "[MultiCut] ");

MultiCut::MultiCut(const Crag& crag, const Parameters& parameters) :
	_crag(crag),
	_cut(crag),
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
MultiCut::setNodeWeights(const Crag::NodeMap<double>& nodeWeights) {

	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
		_objective->setCoefficient(
				nodeIdToVar(_crag.id(n)),
				nodeWeights[n]);
}

void
MultiCut::setEdgeWeights(const Crag::EdgeMap<double>& edgeWeights) {

	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e)
		_objective->setCoefficient(
				edgeIdToVar(_crag.id(e)),
				edgeWeights[e]);
}

MultiCut::Status
MultiCut::solve(unsigned int numIterations) {

	if (numIterations == 0)
		numIterations = _parameters.numIterations;

	for (int i = 0; i < numIterations; i++) {

		LOG_DEBUG(multicutlog)
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

	_solver->setInput("objective", _objective);
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

	LOG_DEBUG(multicutlog)
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

	LOG_DEBUG(multicutlog)
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

		_cut[e] = ((*_solution)[edgeIdToVar(_crag.id(e))] > 0);

		LOG_ALL(multicutlog)
				<< "(" << _crag.id(_crag.u(e))
				<< "," << _crag.id(_crag.v(e))
				<< "): " << _cut[e]
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
		if (_cut[e])
			cutGraph.addEdge(_crag.u(e), _crag.v(e));

	// setup Dijkstra
	One one;
	lemon::Dijkstra<Cut, One> dijkstra(cutGraph, one);

	// for each not selected edge, find the shortest path along connected nodes 
	// connecting them
	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e) {

		if (_cut[e])
			continue;

		Crag::Node s = _crag.u(e);
		Crag::Node t = _crag.v(e);

		LOG_ALL(multicutlog)
				<< "checking cut edge " << _crag.id(s)
				<< ", " << _crag.id(t)
				<< " for cycle consistency"
				<< std::endl;

		// e = (u, v) was not selected -> there should be no path connecting u 
		// and v
		if (dijkstra.run(s, t)) {

			LOG_ALL(multicutlog)
					<< "nodes " << _crag.id(s)
					<< " and " << _crag.id(t)
					<< " are connected"
					<< std::endl;

			LinearConstraint cycleConstraint;

			int lenPath = 0;

			// walk along the path between u and v
			Crag::Node cur = t;
			while (cur != s) {

				LOG_ALL(multicutlog)
						<< "walking backwards: "
						<< _crag.id(cur)
						<< std::endl;

				Crag::Node pre = cutGraph.oppositeNode(cur, dijkstra.predMap()[cur]);

				// here we have to iterate over all adjacent edges in order to 
				// find (cur, pre) in CRAG, since there is no 1:1 mapping 
				// between edges in cutGraph and _crag
				for (Crag::IncEdgeIt pathEdge(_crag, cur); pathEdge != lemon::INVALID; ++pathEdge)
					if (_crag.getAdjacencyGraph().oppositeNode(cur, pathEdge) == pre) {

						cycleConstraint.setCoefficient(
								edgeIdToVar(_crag.id(pathEdge)),
								1.0);
						break;
					}

				lenPath++;
				cur = pre;
			}

			cycleConstraint.setCoefficient(
					edgeIdToVar(_crag.id(e)),
					-1.0);
			cycleConstraint.setRelation(LessEqual);
			cycleConstraint.setValue(lenPath - 1);

			_constraints->add(cycleConstraint);

			constraintsAdded++;

			if (_parameters.maxConstraintsPerIteration > 0 &&
			    constraintsAdded >= _parameters.maxConstraintsPerIteration)
				break;
		}
	}

	LOG_DEBUG(multicutlog)
			<< "added " << constraintsAdded
			<< " cycle constraints" << std::endl;

	return (constraintsAdded > 0);
}
