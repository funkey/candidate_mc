#include "DownSampler.h"
#include <util/Logger.h>
#include <util/timing.h>

logger::LogChannel downsamplerlog("downsamplerlog", "[DownSampler] ");

void
DownSampler::process(const Crag& crag, Crag* downSampled) {

	LOG_USER(downsamplerlog) << "downsampling CRAG..." << std::endl;

	UTIL_TIME_METHOD;

	_copyMap.clear();
	_sizes.clear();

	// downsample the graph
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		if (crag.isRootNode(n)) {

			Crag::SubsetNode root = crag.toSubset(n);

			LOG_DEBUG(downsamplerlog)
					<< "downsampling below root node " << crag.id(root)
					<< std::endl;

			downSampleCopy(
					crag,
					root, /* root nodes are always valid parents... */
					root,
					false, /* ...and not single children */
					*downSampled);
		}
	}

	// make sure all leaf nodes have a valid volume
	for (Crag::SubsetNodeIt n(crag); n != lemon::INVALID; ++n) {

		if (!_copyMap.count(n))
			continue;

		Crag::Node copy = _copyMap[n];

		if (downSampled->isLeafNode(copy))
			downSampled->getVolumeMap()[copy] = crag.getVolume(crag.toRag(n));
	}

	int numNodes = 0;
	for (Crag::NodeIt n(*downSampled); n != lemon::INVALID; ++n)
		numNodes++;

	LOG_USER(downsamplerlog)
			<< "downsampled CRAG contains "
			<< numNodes << " nodes"
			<< std::endl;
}

void
DownSampler::downSampleCopy(const Crag& source, Crag::SubsetNode parent, Crag::SubsetNode n, bool singleChild, Crag& target) {

	// parent is the last valid parent node, i.e., a node with more than one 
	// valid children (size >= _minSize) or a root node
	//
	// n is the current traversal node below parent
	//
	// singleChild is true, if n is a single child

	bool debug = (source.id(n) == 21848);

	// if n is too small, there is nothing to copy
	if (!source.isRootNode(source.toRag(n)) && size(source, n) < _minSize)
		return;

	if (debug) LOG_ALL(downsamplerlog) << "processing root node" << std::endl;

	// n is valid and not a single child -- copy it to the target graph
	if (!singleChild) {

		if (debug) LOG_ALL(downsamplerlog) << "is not a single child" << std::endl;

		Crag::Node copy = target.addNode();
		_copyMap[n] = copy;

		if (debug) LOG_ALL(downsamplerlog) << "is node " << target.id(copy) << " in copy" << std::endl;

		// for the first call, root == parent == n
		if (n != parent) {

			if (debug) LOG_ALL(downsamplerlog) << "adding subset arc to " << target.id(_copyMap[parent]) << std::endl;
			target.addSubsetArc(copy, _copyMap[parent]);
		}

		// n is now the previous valid parent
		parent = n;
	}

	int numChildren = 0;
	for (Crag::SubsetInArcIt childEdge(source, n); childEdge != lemon::INVALID; ++childEdge)
		numChildren++;

	for (Crag::SubsetInArcIt childEdge(source, n); childEdge != lemon::INVALID; ++childEdge)
		downSampleCopy(
				source,
				parent,
				source.getSubsetGraph().oppositeNode(n, childEdge),
				(numChildren == 1),
				target);

	for (Crag::SubsetInArcIt childEdge(target, target.toSubset(_copyMap[n])); childEdge != lemon::INVALID; ++childEdge)
		if (debug) LOG_ALL(downsamplerlog) << "has a child" << std::endl;
	for (Crag::SubsetOutArcIt parEdge(target, target.toSubset(_copyMap[n])); parEdge != lemon::INVALID; ++parEdge)
		if (debug) LOG_ALL(downsamplerlog) << "has a parent...?" << std::endl;
}

int
DownSampler::size(const Crag& crag, Crag::SubsetNode n) {

	UTIL_TIME_METHOD;

	if (_sizes.count(n))
		return _sizes[n];

	int nodeSize = 0;

	if (crag.isLeafNode(crag.toRag(n))) {

		const ExplicitVolume<unsigned char>& volume = crag.getVolume(crag.toRag(n));

		for (auto& p : volume.data())
			if (p)
				nodeSize++;

		return nodeSize;
	}

	for (Crag::SubsetInArcIt childEdge(crag, n); childEdge != lemon::INVALID; ++childEdge)
		nodeSize += size(crag, crag.getSubsetGraph().oppositeNode(n, childEdge));

	_sizes[n] = nodeSize;

	return nodeSize;
}
