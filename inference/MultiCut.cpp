#include <lemon/dijkstra.h>
#include <lemon/connectivity.h>
#include <util/Logger.h>
#include "MultiCut.h"

#include <vigra/multi_impex.hxx>
#include <vigra/functorexpression.hxx>

logger::LogChannel multicutlog("multicutlog", "[MultiCut] ");

MultiCut::MultiCut(const Crag& crag, const Parameters& parameters) :
	_crag(crag),
	_merged(crag),
	_selected(crag),
	_labels(crag),
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
MultiCut::setCosts(const Costs& costs) {

	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
		_objective->setCoefficient(
				nodeIdToVar(_crag.id(n)),
				costs.node[n]);

	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e)
		_objective->setCoefficient(
				edgeIdToVar(_crag.id(e)),
				costs.edge[e]);
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

			LOG_USER(multicutlog)
					<< "optimal solution with value "
					<< _solution->getValue() << " found"
					<< std::endl;

			return SolutionFound;
		}
	}

	LOG_USER(multicutlog) << "maximum number of iterations reached" << std::endl;
	return MaxIterationsReached;
}

void
MultiCut::storeSolution(const std::string& filename, bool boundary) {

	util::box<float, 3>   cragBB = _crag.getBoundingBox();
	util::point<float, 3> resolution;
	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

		if (_crag.getVolumes()[n].getDiscreteBoundingBox().isZero())
			continue;
		resolution = _crag.getVolumes()[n].getResolution();
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

	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

		if (_crag.getVolumes()[n].getDiscreteBoundingBox().isZero())
			continue;

		const util::point<float, 3>&      volumeOffset     = _crag.getVolumes()[n].getOffset();
		const util::box<unsigned int, 3>& volumeDiscreteBB = _crag.getVolumes()[n].getDiscreteBoundingBox();

		util::point<unsigned int, 3> begin = (volumeOffset - cragBB.min())/resolution;
		util::point<unsigned int, 3> end   = begin +
				util::point<unsigned int, 3>(
						volumeDiscreteBB.width(),
						volumeDiscreteBB.height(),
						volumeDiscreteBB.depth());

		// fill id of connected component
		vigra::combineTwoMultiArrays(
				_crag.getVolumes()[n].data(),
				components.subarray(
						vigra::Shape3(
								begin.x(),
								begin.y(),
								begin.z()),
						vigra::Shape3(
								end.x(),
								end.y(),
								end.z())),
				components.subarray(
						vigra::Shape3(
								begin.x(),
								begin.y(),
								begin.z()),
						vigra::Shape3(
								end.x(),
								end.y(),
								end.z())),
				vigra::functor::ifThenElse(
						vigra::functor::Arg1() == vigra::functor::Param(1),
						vigra::functor::Param(_labels[n] + 1),
						vigra::functor::Arg2()
				));

	}

	if (boundary) {

		// gray boundary for all leaf nodes
		for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
			if (Crag::SubsetInArcIt(_crag, _crag.toSubset(n)) == lemon::INVALID)
				drawBoundary(n, components, 0.5);

		// black boundary for all selected nodes
		for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
			if (_selected[n])
				drawBoundary(n, components, 0);
	}

	vigra::exportImage(
			components.bind<2>(0),
			vigra::ImageExportInfo(filename.c_str()));
}

void
MultiCut::prepareSolver() {

	LOG_DEBUG(multicutlog) << "preparing solver..." << std::endl;

	// one binary indicator per node and edge
	_objective->resize(_numNodes + _numEdges);
	_objective->setSense(_parameters.minimize ? Minimize : Maximize);
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
	lemon::connectedComponents(cutGraph, _labels);

	// label rejected nodes with -1
	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

		if ((*_solution)[nodeIdToVar(_crag.id(n))] < 0.5) {

			_selected[n]  = false;
			_labels[n] = -1;

		} else {

			_selected[n] = true;
		}

		LOG_ALL(multicutlog)
				<< _crag.id(n) << ": "
				<< _selected[n]
				<< std::endl;
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
		if (_labels[s] != _labels[t] || !_selected[s])
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

	// propagate node labels to subsets
	for (Crag::SubsetNodeIt n(_crag); n != lemon::INVALID; ++n) {

		int numParents = 0;
		for (Crag::SubsetOutArcIt e(_crag, n); e != lemon::INVALID; ++e)
			numParents++;

		if (numParents == 0)
			propagateLabel(n, -1);
	}

	return (constraintsAdded > 0);
}

void
MultiCut::propagateLabel(Crag::SubsetNode n, int label) {

	if (label == -1)
		label = _labels[_crag.toRag(n)];
	else
		_labels[_crag.toRag(n)] = label;

	for (Crag::SubsetInArcIt e(_crag, n); e != lemon::INVALID; ++e)
		propagateLabel(_crag.getSubsetGraph().oppositeNode(n, e), label);
}

void
MultiCut::drawBoundary(
		Crag::Node                   n,
		vigra::MultiArray<3, float>& components,
		float                        value) {

	ExplicitVolume<bool> volume = getVolume(_crag.toSubset(n));
	const util::box<unsigned int, 3>& volumeDiscreteBB = volume.getDiscreteBoundingBox();
	const util::point<float, 3>&      volumeOffset     = volume.getOffset();
	util::point<unsigned int, 3>      begin            = (volumeOffset - _crag.getBoundingBox().min())/volume.getResolution();

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

ExplicitVolume<bool>
MultiCut::getVolume(Crag::SubsetNode n) {

	// bounding box of the volume of node n
	util::box<float, 3> bb;

	// volumes of all children
	std::vector<ExplicitVolume<bool>> childVolumes;

	// resolution of volumes
	util::point<float, 3> resolution;

	unsigned int children = 0;
	for (Crag::SubsetInArcIt e(_crag.getSubsetGraph(), n); e != lemon::INVALID; ++e) {

		Crag::SubsetNode child = _crag.getSubsetGraph().source(e);

		childVolumes.push_back(getVolume(child));
		bb.fit(childVolumes.back().getBoundingBox());

		if (resolution.isZero()) {

			resolution = childVolumes.back().getResolution();

		} else {

			if (resolution != childVolumes.back().getResolution())
				UTIL_THROW_EXCEPTION(
						Exception,
						"volumes in CRAG have different resolutions");
		}

		children++;
	}

	// we are a leaf node
	if (children == 0)
		return _crag.getVolumes()[_crag.toRag(n)];

	util::box<unsigned int, 3> dbb = (bb - bb.min())/resolution;

	ExplicitVolume<bool> volume(dbb.width(), dbb.height(), dbb.depth());
	volume.setResolution(resolution);
	volume.setOffset(bb.min());

	for (const ExplicitVolume<bool>& childVolume : childVolumes) {

		util::box<unsigned int, 3>   childDbb     = childVolume.getDiscreteBoundingBox();
		util::point<unsigned int, 3> childDOffset = (childVolume.getOffset() - volume.getOffset())/resolution;

		LOG_ALL(multicutlog) << "child size " << childDbb << std::endl;

		for (unsigned int z = 0; z < childDbb.depth();  z++)
		for (unsigned int y = 0; y < childDbb.height(); y++)
		for (unsigned int x = 0; x < childDbb.width();  x++) {

			LOG_ALL(multicutlog) << "testing child " << x << ", " << y << ", " << z << std::endl;
			if (childVolume(x, y, z))
				volume.data()(
						childDOffset.x() + x,
						childDOffset.y() + y,
						childDOffset.z() + z) = true;
		}
	}

	return volume;
}

