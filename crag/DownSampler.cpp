#include "DownSampler.h"
#include <util/Logger.h>
#include <util/timing.h>

logger::LogChannel downsamplerlog("downsamplerlog", "[DownSampler] ");

void
DownSampler::process(const Crag& crag, const CragVolumes& volumes, Crag& downSampled, CragVolumes& downSampledVolumes) {

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
					volumes,
					root, /* root nodes are always valid parents... */
					root,
					false, /* ...and not single children */
					downSampled);
		}
	}

	// make sure all copied nodes have a valid volume
	unsigned int numOriginalNodes = 0;
	unsigned int numDownsampledNodes = 0;
	for (Crag::SubsetNodeIt n(crag); n != lemon::INVALID; ++n) {

		numOriginalNodes++;

		if (!_copyMap.count(n))
			continue;

		numDownsampledNodes++;
		Crag::Node copy = _copyMap[n];

		downSampledVolumes.setVolume(copy, volumes[crag.toRag(n)]);
	}

	LOG_USER(downsamplerlog)
			<< "downsampled CRAG contains "
			<< numDownsampledNodes << " nodes, "
			<< (numOriginalNodes - numDownsampledNodes) << " less "
			<< "then original CRAG"
			<< std::endl;
}

void
DownSampler::downSampleCopy(const Crag& source, const CragVolumes& sourceVolumes, Crag::SubsetNode parent, Crag::SubsetNode n, bool singleChild, Crag& target) {

	// parent is the last valid parent node, i.e., a node with more than one 
	// valid children (size >= _minSize) or a root node
	//
	// n is the current traversal node below parent
	//
	// singleChild is true, if n is a single child

	// if n is too small, there is nothing to copy
	if (!source.isRootNode(source.toRag(n)) && size(source, sourceVolumes, n) < _minSize)
		return;

	// n is valid and not a single child -- copy it to the target graph
	if (!singleChild) {

		Crag::Node copy = target.addNode();
		_copyMap[n] = copy;

		// for the first call, root == parent == n
		if (n != parent)
			target.addSubsetArc(copy, _copyMap[parent]);

		// n is now the previous valid parent
		parent = n;
	}

	int numChildren = 0;
	for (Crag::SubsetInArcIt childEdge(source, n); childEdge != lemon::INVALID; ++childEdge)
		numChildren++;

	for (Crag::SubsetInArcIt childEdge(source, n); childEdge != lemon::INVALID; ++childEdge)
		downSampleCopy(
				source,
				sourceVolumes,
				parent,
				source.getSubsetGraph().oppositeNode(n, childEdge),
				(numChildren == 1),
				target);
}

int
DownSampler::size(const Crag& crag, const CragVolumes& volumes, Crag::SubsetNode n) {

	UTIL_TIME_METHOD;

	if (_sizes.count(n))
		return _sizes[n];

	int nodeSize = 0;
	for (auto& p : volumes[crag.toRag(n)]->data())
		if (p)
			nodeSize++;
	_sizes[n] = nodeSize;

	return nodeSize;
}
