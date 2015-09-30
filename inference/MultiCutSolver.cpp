#include <boost/filesystem.hpp>
#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>
#include <solver/SolverFactory.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/box.hpp>
#include "MultiCutSolver.h"

#include <vigra/multi_impex.hxx>
#include <vigra/functorexpression.hxx>

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
	CragSolver(crag),
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
MultiCutSolver::solve() {

	_solver->setObjective(_objective);

	for (unsigned int i = 0; i < _parameters.numIterations; i++) {

		LOG_USER(multicutlog)
				<< "------------------------ iteration "
				<< i << std::endl;

		findCut();

		if (!findViolatedConstraints()) {

			LOG_USER(multicutlog)
					<< "optimal solution with value "
					<< _solution.getValue() << " found"
					<< std::endl;

			int numSelected = 0;
			int numMerged = 0;
			for (Crag::CragNode n : _crag.nodes())
				if (_cragSolution.selected(n))
					numSelected++;
			for (Crag::CragEdge e : _crag.edges())
				if (_cragSolution.selected(e))
					numMerged++;

			LOG_USER(multicutlog)
					<< numSelected << " candidates selected, "
					<< numMerged << " adjacent candidates merged"
					<< std::endl;

			return SolutionFound;
		}
	}

	LOG_USER(multicutlog) << "maximum number of iterations reached" << std::endl;
	return MaxIterationsReached;
}

void
MultiCutSolver::storeSolution(const CragVolumes& volumes, const std::string& filename, bool boundary) {

	util::box<float, 3>   cragBB = volumes.getBoundingBox();
	util::point<float, 3> resolution;
	for (Crag::CragNode n : _crag.nodes()) {

		if (!_crag.isLeafNode(n))
			continue;
		resolution = volumes[n]->getResolution();
		break;
	}

	LOG_ALL(multicutlog) << "CRAG bounding box is " << cragBB << std::endl;
	LOG_ALL(multicutlog) << "CRAG resolution from volumes is " << resolution << std::endl;

	// create a vigra multi-array large enough to hold all volumes
	vigra::MultiArray<3, float> components(
			vigra::Shape3(
				cragBB.width() /resolution.x(),
				cragBB.height()/resolution.y(),
				cragBB.depth() /resolution.z()),
			std::numeric_limits<int>::max());

	// background for areas without candidates
	if (boundary)
		components = 0.25;
	else
		components = 0;

	for (Crag::CragNode n : _crag.nodes()) {

		if (boundary) {

			// draw only leaf nodes if we want to visualize the boundary
			if (!_crag.isLeafNode(n))
				continue;

		} else {

			// otherwise, draw only selected nodes
			if (!_cragSolution.selected(n))
				continue;
		}

		const util::point<float, 3>&      volumeOffset     = volumes[n]->getOffset();
		const util::box<unsigned int, 3>& volumeDiscreteBB = volumes[n]->getDiscreteBoundingBox();


		util::point<unsigned int, 3> begin = (volumeOffset - cragBB.min())/resolution;
		util::point<unsigned int, 3> end   = begin +
				util::point<unsigned int, 3>(
						volumeDiscreteBB.width(),
						volumeDiscreteBB.height(),
						volumeDiscreteBB.depth());

		// fill id of connected component
		vigra::combineTwoMultiArrays(
			volumes[n]->data(),
			components.subarray(TinyVector3UInt(&begin[0]),TinyVector3UInt(&end[0])),
			components.subarray(TinyVector3UInt(&begin[0]),TinyVector3UInt(&end[0])),
			vigra::functor::ifThenElse(
					vigra::functor::Arg1() == vigra::functor::Param(1),
					vigra::functor::Param(_labels[n] + 1),
					vigra::functor::Arg2()
		));
	}

	if (boundary) {

		// gray boundary for all leaf nodes
		for (Crag::CragNode n : _crag.nodes())
			if (_crag.isLeafNode(n))
				drawBoundary(volumes, n, components, 0.5);

		// black boundary for all selected nodes
		for (Crag::CragNode n : _crag.nodes())
			if (_cragSolution.selected(n))
				drawBoundary(volumes, n, components, 0);
	}

	if (components.shape(2) > 1) {

		boost::filesystem::create_directory(filename);
		for (unsigned int z = 0; z < components.shape(2); z++) {

			std::stringstream ss;
			ss << std::setw(4) << std::setfill('0') << z;
			vigra::exportImage(
					components.bind<2>(z),
					vigra::ImageExportInfo((filename + "/" + ss.str() + ".tif").c_str()));
		}

	} else {

		vigra::exportImage(
				components.bind<2>(0),
				vigra::ImageExportInfo(filename.c_str()));
	}
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

		std::vector<Crag::Edge> childEdges;

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
		for (Crag::Edge e : childEdges) {

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
MultiCutSolver::findCut() {

	// re-set constraints to inform solver about potential changes
	_solver->setConstraints(_constraints);
	std::string msg;
	if (!_solver->solve(_solution, msg))
		LOG_ERROR(multicutlog) << "solver did not find optimal solution: " << msg << std::endl;

	LOG_USER(multicutlog) << "searching for optimal cut..." << std::endl;

	// get selected candidates
	for (Crag::CragNode n : _crag.nodes()) {

		_cragSolution.setSelected(n, (_solution[nodeIdToVar(_crag.id(n))] > 0.5));

		LOG_ALL(multicutlog)
				<< _crag.id(n) << ": "
				<< _cragSolution.selected(n)
				<< std::endl;
	}

	// get merged edges
	for (Crag::CragEdge e : _crag.edges()) {

		_cragSolution.setSelected(e, (_solution[edgeIdToVar(_crag.id(e))] > 0.5));

		LOG_ALL(multicutlog)
				<< "(" << _crag.id(_crag.u(e))
				<< "," << _crag.id(_crag.v(e))
				<< "): " << _cragSolution.selected(e)
				<< std::endl;
	}
}

bool
MultiCutSolver::findViolatedConstraints() {

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
		if (_cragSolution.selected(e))
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
		if (!_cragSolution.selected(n))
			_labels[n] = -1;

	// for each not selected edge with nodes in the same connected component, 
	// find the shortest path along connected nodes connecting them
	for (Crag::CragEdge e : _crag.edges()) {

		// not selected
		if (_cragSolution.selected(e))
			continue;

		Crag::CragNode s = e.u();
		Crag::CragNode t = e.v();

		// in same component
		if (_labels[s] != _labels[t] || !_cragSolution.selected(s))
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
				<< _labels[_crag.v(e)]
				<< std::endl;

		LinearConstraint cycleConstraint;

		int lenPath = 0;

		// walk along the path between u and v
		Crag::CragNode cur = t;
		LOG_DEBUG(multicutlog)
				<< "nodes " << _crag.id(s)
				<< " and " << _crag.id(t)
				<< " (edge " << edgeIdToVar(_crag.id(e)) << ")"
				<< " are connected via path ";
		while (cur != s) {

			LOG_DEBUG(multicutlog)
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

			if (!_cragSolution.selected(*pathEdge))
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
			LOG_DEBUG(multicutlog)
					<< "(edge " << edgeIdToVar(_crag.id(*pathEdge)) << ") ";

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

void
MultiCutSolver::drawBoundary(
		const CragVolumes&           volumes,
		Crag::Node                   n,
		vigra::MultiArray<3, float>& components,
		float                        value) {

	const CragVolume& volume = *volumes[n];
	const util::box<unsigned int, 3>& volumeDiscreteBB = volume.getDiscreteBoundingBox();
	const util::point<float, 3>&      volumeOffset     = volume.getOffset();
	util::point<unsigned int, 3>      begin            = (volumeOffset - volumes.getBoundingBox().min())/volume.getResolution();

	bool hasZ = (volumeDiscreteBB.depth() > 1);

	// draw boundary
	for (unsigned int z = 0; z < volumeDiscreteBB.depth();  z++)
	for (unsigned int y = 0; y < volumeDiscreteBB.height(); y++)
	for (unsigned int x = 0; x < volumeDiscreteBB.width();  x++) {

		// only inside voxels
		if (!volume.data()(x, y, z))
			continue;

		if ((hasZ && (z == 0 || z == volumeDiscreteBB.depth()  - 1)) ||
			y == 0 || y == volumeDiscreteBB.height() - 1 ||
			x == 0 || x == volumeDiscreteBB.width()  - 1) {

			components(
					begin.x() + x,
					begin.y() + y,
					begin.z() + z) = value;
			continue;
		}

		bool done = false;
		for (int dz = (hasZ ? -1 : 0); dz <= (hasZ ? 1 : 0) && !done; dz++)
		for (int dy = -1; dy <= 1 && !done; dy++)
		for (int dx = -1; dx <= 1 && !done; dx++) {

			if (std::abs(dx) + std::abs(dy) + std::abs(dz) > 1)
				continue;

			if (!volume.data()(x + dx, y + dy, z + dz)) {

				components(
						begin.x() + x,
						begin.y() + y,
						begin.z() + z) = value;
				done = true;
			}
		}
	}
}

