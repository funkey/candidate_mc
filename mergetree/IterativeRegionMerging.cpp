#include <cassert>
#include <util/Logger.h>
#include "IterativeRegionMerging.h"

logger::LogChannel mergetreelog("mergetreelog", "[IterativeRegionMerging] ");

namespace vigra {

// type traits to use MultiArrayView as NodeMap
template <>
struct GraphMapTypeTraits<MultiArrayView<2, int> > {
	typedef int        Value;
	typedef int&       Reference;
	typedef const int& ConstReference;
};

} // namespace vigra

IterativeRegionMerging::IterativeRegionMerging(
		vigra::MultiArrayView<2, int> initialRegions) :
	_grid(initialRegions.shape()),
	_gridEdgeWeights(_grid),
	_ragToGridEdges(_rag),
	_parentNodes(_rag),
	_edgeScores(_rag),
	_mergeTree(initialRegions.shape()*2),
	_mergeEdges(EdgeCompare(_edgeScores)) {

	// get initial region adjecancy graph

	RagType::EdgeMap<std::vector<GridGraphType::Edge> > affiliatedEdges;
	vigra::makeRegionAdjacencyGraph(
			_grid,
			initialRegions,
			_rag,
			affiliatedEdges);

	// get grid edges for each rag edge

	for (RagType::EdgeIt edge(_rag); edge != lemon::INVALID; ++edge)
		_ragToGridEdges[*edge] = affiliatedEdges[*edge];

	// prepare merge-tree image

	_mergeTree = 0;

	// logging

	int numRegions = 0;
	for (RagType::NodeIt node(_rag); node != lemon::INVALID; ++node)
		numRegions++;
	int numRegionEdges = _ragToGridEdges.size();

	LOG_USER(mergetreelog)
			<< "got region adjecancy graph with "
			<< numRegions << " regions and "
			<< numRegionEdges << " edges" << std::endl;
}

void
IterativeRegionMerging::finishMergeTree() {

	LOG_USER(mergetreelog) << "finishing merge tree image..." << std::endl;

	// get the max leaf distance for each region

	util::cont_map<RagType::Node, int, NodeNumConverter<RagType> > leafDistances(_rag);

	int maxDistance = 0;
	for (RagType::NodeIt node(_rag); node != lemon::INVALID; ++node) {

		int distance;
		RagType::Node parent = *node;
		for (distance = 0; parent != lemon::INVALID; distance++, parent = _parentNodes[parent]) {

			if (!leafDistances.count(parent))
				leafDistances[parent] = distance;
			else {

				if (distance > leafDistances[parent])
					leafDistances[parent] = distance;
				else
					break;
			}
		}

		maxDistance = std::max(distance, maxDistance);
	}

	// with too many merge levels we run into trouble later
	UTIL_ASSERT_REL(maxDistance, <, 65535);

	LOG_DEBUG(mergetreelog) << "max merge-tree depth is " << maxDistance << std::endl;

	// replace region ids in merge tree image with leaf distance
	for (vigra::MultiArray<2, int>::iterator i = _mergeTree.begin(); i != _mergeTree.end(); i++)
		*i = leafDistances.at_index(*i);
}
