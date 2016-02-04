#ifndef MULTI2CUT_MERGETREE_ITERATIVE_REGION_MERGING_H__
#define MULTI2CUT_MERGETREE_ITERATIVE_REGION_MERGING_H__

#include <iostream>
#include <fstream>
#include <util/Logger.h>
#include <util/cont_map.hpp>
#include <util/assert.h>
#include "NodeNumConverter.h"
#include "EdgeNumConverter.h"
#include <vigra/multi_gridgraph.hxx>
#include <vigra/multi_watersheds.hxx>
#include <vigra/adjacency_list_graph.hxx>
#include <vigra/graph_algorithms.hxx>

extern logger::LogChannel mergetreelog;

namespace vigra {

// type traits to use MultiArrayView as NodeMap
template <>
struct GraphMapTypeTraits<MultiArrayView<2, int> > {
	typedef int        Value;
	typedef int&       Reference;
	typedef const int& ConstReference;
};

// type traits to use MultiArrayView as NodeMap
template <>
struct GraphMapTypeTraits<MultiArrayView<3, int> > {
	typedef int        Value;
	typedef int&       Reference;
	typedef const int& ConstReference;
};

} // namespace vigra


template <int D>
class IterativeRegionMerging {

public:

	typedef vigra::GridGraph<D>       GridGraphType;
	typedef vigra::AdjacencyListGraph RagType;

	IterativeRegionMerging(vigra::MultiArrayView<D, int> initialRegions);

	/**
	 * Store the initial (before calling createMergeTree) or final RAG.
	 */
	template <typename ScoringFunction>
	void storeRag(std::string filename, ScoringFunction& scoringFunction);

	template <typename ScoringFunction>
	void createMergeTree(ScoringFunction& scoringFunction);

	void storeMergeHistory(std::string filename);

	/**
	 * Get the region adjacency graph.
	 */
	RagType& getRag() { return _rag; }

private:

	typedef util::cont_map<RagType::Edge, std::vector<typename GridGraphType::Edge>, EdgeNumConverter<RagType> > GridEdgesType;
	typedef util::cont_map<RagType::Node, RagType::Node, NodeNumConverter<RagType> >                    ParentNodesType;
	typedef util::cont_map<RagType::Edge, float, EdgeNumConverter<RagType> >                            EdgeScoresType;

	struct EdgeCompare {

		EdgeCompare(const EdgeScoresType& edgeScores_) :
			edgeScores(edgeScores_) {}

		bool operator()(const RagType::Edge& a, const RagType::Edge& b) {

			UTIL_ASSERT(edgeScores.count(a));
			UTIL_ASSERT(edgeScores.count(b));

			// sort edges in increasing score value
			return edgeScores[a] > edgeScores[b];
		}

		const EdgeScoresType& edgeScores;
	};

	typedef std::priority_queue<RagType::Edge, std::vector<RagType::Edge>, EdgeCompare> MergeEdgesType;

	// merge two regions and return new region node
	template <typename ScoringFunction>
	RagType::Node mergeRegions(
			RagType::Node regionA,
			RagType::Node regionB,
			ScoringFunction& scoringFunction);

	// merge two regions identified by an edge
	// a and b can optionally be given as u and v of the edge to avoid a lookup
	template <typename ScoringFunction>
	RagType::Node mergeRegions(
			RagType::Edge edge,
			ScoringFunction& scoringFunction,
			RagType::Node a = RagType::Node(),
			RagType::Node b = RagType::Node());

	template <typename ScoringFunction>
	void scoreEdge(const RagType::Edge& edge, ScoringFunction& scoringFunction);

	inline RagType::Edge nextMergeEdge() { float _; return nextMergeEdge(_); }
	inline RagType::Edge nextMergeEdge(float& score);

	void finishMergeTree();

	GridGraphType                 _grid;
	typename GridGraphType::template EdgeMap<float> _gridEdgeWeights;

	RagType _rag;

	GridEdgesType   _ragToGridEdges;
	ParentNodesType _parentNodes;
	EdgeScoresType  _edgeScores;

	struct Merge {

		typename RagType::Node u;
		typename RagType::Node v;
		typename RagType::Node parent;
		float                  score;
	};
	std::vector<Merge> _mergeHistory;

	MergeEdgesType _mergeEdges;
};

template <int D>
IterativeRegionMerging<D>::IterativeRegionMerging(
		vigra::MultiArrayView<D, int> initialRegions) :
	_grid(initialRegions.shape()),
	_gridEdgeWeights(_grid),
	_ragToGridEdges(_rag),
	_parentNodes(_rag),
	_edgeScores(_rag),
	_mergeEdges(EdgeCompare(_edgeScores)) {

	// get initial region adjacency graph

	RagType::EdgeMap<std::vector<typename GridGraphType::Edge> > affiliatedEdges;
	vigra::makeRegionAdjacencyGraph(
			_grid,
			initialRegions,
			_rag,
			affiliatedEdges);

	// get grid edges for each rag edge

	for (RagType::EdgeIt edge(_rag); edge != lemon::INVALID; ++edge)
		_ragToGridEdges[*edge] = affiliatedEdges[*edge];

	// logging

	int numRegions = 0;
	for (RagType::NodeIt node(_rag); node != lemon::INVALID; ++node)
		numRegions++;
	int numRegionEdges = _ragToGridEdges.size();

	LOG_USER(mergetreelog)
			<< "got region adjacency graph with "
			<< numRegions << " regions and "
			<< numRegionEdges << " edges" << std::endl;
}

template <int D>
template <typename ScoringFunction>
void
IterativeRegionMerging<D>::storeRag(std::string filename, ScoringFunction& scoringFunction) {

	std::ofstream file(filename.c_str());

	for (RagType::EdgeIt edge(_rag); edge != lemon::INVALID; ++edge) {

		unsigned int u = _rag.id(_rag.u(*edge));
		unsigned int v = _rag.id(_rag.v(*edge));

		file << u << "\t" << v << "\t" << scoringFunction(*edge, _ragToGridEdges[*edge]) << std::endl;
	}
}

template <int D>
template <typename ScoringFunction>
void
IterativeRegionMerging<D>::createMergeTree(ScoringFunction& scoringFunction) {

	LOG_USER(mergetreelog) << "computing initial edge scores..." << std::endl;

	// compute initial edge scores
	for (RagType::EdgeIt edge(_rag); edge != lemon::INVALID; ++edge)
		scoreEdge(*edge, scoringFunction);

	LOG_USER(mergetreelog) << "merging regions..." << std::endl;

	RagType::Edge next;
	float         score;

	while (true) {

		next = nextMergeEdge(score);
		if (next == lemon::INVALID)
			break;

		RagType::Node merged = mergeRegions(next, scoringFunction);

		_mergeHistory.push_back({_rag.u(next), _rag.v(next), merged, score});

		LOG_ALL(mergetreelog)
				<< "merged regions " << _rag.id(_rag.u(next)) << " and " << _rag.id(_rag.v(next))
				<< " with score " << score
				<< " into " << _rag.id(merged) << std::endl;
	}

	LOG_USER(mergetreelog) << "finished merging" << std::endl;
	LOG_DEBUG(mergetreelog)
			<< "_ragToGridEdges contains "
			<< _ragToGridEdges.size() << " elements, with an overhead of "
			<< _ragToGridEdges.overhead() << std::endl;
}

template <int D>
void
IterativeRegionMerging<D>::storeMergeHistory(std::string filename) {

	std::ofstream file(filename.c_str());

	for (const Merge& m : _mergeHistory) {

		unsigned int u = _rag.id(m.u);
		unsigned int v = _rag.id(m.v);
		unsigned int p = _rag.id(m.parent);

		// vigra starts counting region ids with 1, but we export images where 
		// we start with 0, therefore:
		UTIL_ASSERT_REL(u, >, 0);
		UTIL_ASSERT_REL(v, >, 0);
		UTIL_ASSERT_REL(p, >, 0);
		u -= 1;
		v -= 1;
		p -= 1;

		file << u << "\t" << v << "\t" << p << "\t" << m.score << std::endl;
	}
}

template <int D>
template <typename ScoringFunction>
IterativeRegionMerging<D>::RagType::Node
IterativeRegionMerging<D>::mergeRegions(
		RagType::Node a,
		RagType::Node b,
		ScoringFunction& scoringFunction) {

	// don't merge previously merged nodes
	UTIL_ASSERT(_parentNodes[a] == lemon::INVALID);
	UTIL_ASSERT(_parentNodes[b] == lemon::INVALID);

	RagType::Edge edge = _rag.findEdge(a, b);

	if (edge == lemon::INVALID)
		return RagType::Node();

	return mergeRegions(edge, scoringFunction, a, b);
}

template <int D>
template <typename ScoringFunction>
IterativeRegionMerging<D>::RagType::Node
IterativeRegionMerging<D>::mergeRegions(
		RagType::Edge edge,
		ScoringFunction& scoringFunction,
		RagType::Node a,
		RagType::Node b) {

	if (a == lemon::INVALID || b == lemon::INVALID) {

		a = _rag.u(edge);
		b = _rag.v(edge);
	}

	// don't merge previously merged nodes
	UTIL_ASSERT(_parentNodes[a] == lemon::INVALID);
	UTIL_ASSERT(_parentNodes[b] == lemon::INVALID);

	// add new c = a + b
	RagType::Node c = _rag.addNode();

	_parentNodes[a] = c;
	_parentNodes[b] = c;

	// connect c to neighbors of a and b and set affiliated edges accordingly

	std::vector<RagType::Edge> newEdges;

	// for child in {a,b}
	for (int i = 0; i <= 1; i++) {
		RagType::Node child = (i == 0 ? a : b);
		RagType::Node other = (i == 0 ? b : a);

		std::vector<RagType::Node> neighbors;
		std::vector<RagType::Edge> neighborEdges;

		// for all edges incident to child
		for (RagType::IncEdgeIt edge(_rag, child); edge != lemon::INVALID; ++edge) {

			// get the neighbor
			RagType::Node neighbor = (_rag.u(*edge) == child ? _rag.v(*edge) : _rag.u(*edge));

			// don't consider the node we currently merge with, and all 
			// previously merged nodes
			if (neighbor == other || _parentNodes[neighbor] != lemon::INVALID)
				continue;

			neighbors.push_back(neighbor);
			neighborEdges.push_back(*edge);
		}

		// for all neighbors
		for (unsigned int i = 0; i < neighbors.size(); i++) {

			const RagType::Node neighbor     = neighbors[i];
			const RagType::Edge neighborEdge = neighborEdges[i];

			// add the edge from c->neighbor
			RagType::Edge newEdge = _rag.findEdge(c, neighbor);
			if (newEdge == lemon::INVALID)
				newEdge = _rag.addEdge(c, neighbor);

			// add affiliated edges from child->neighbor to new edge c->neighbor
			std::copy(
					_ragToGridEdges[neighborEdge].begin(),
					_ragToGridEdges[neighborEdge].end(),
					std::back_inserter(_ragToGridEdges[newEdge]));

			// clear the affiliated edges to save memory -- they are not needed 
			// anymore
			_ragToGridEdges[neighborEdge].clear();

			newEdges.push_back(newEdge);
		}
	}

	// inform visitor
	scoringFunction.onMerge(edge, c);

	// get edge score for new edges
	for (std::vector<RagType::Edge>::const_iterator i = newEdges.begin(); i != newEdges.end(); i++)
		scoreEdge(*i, scoringFunction);

	return c;
}

template <int D>
template <typename ScoringFunction>
void
IterativeRegionMerging<D>::scoreEdge(const RagType::Edge& edge, ScoringFunction& scoringFunction) {

	_edgeScores[edge] = scoringFunction(edge, _ragToGridEdges[edge]);
	_mergeEdges.push(edge);
}

template <int D>
IterativeRegionMerging<D>::RagType::Edge
IterativeRegionMerging<D>::nextMergeEdge(float& score) {

	RagType::Edge next;

	while (true) {

		// no more edges
		if (_mergeEdges.size() == 0)
			return RagType::Edge();

		next = _mergeEdges.top(); _mergeEdges.pop();

		// don't accept edges to already merged regions
		if (_parentNodes[_rag.u(next)] != lemon::INVALID || _parentNodes[_rag.v(next)] != lemon::INVALID)
			continue;

		break;
	}

	score = _edgeScores[next];

	return next;
}

#endif // MULTI2CUT_MERGETREE_ITERATIVE_REGION_MERGING_H__

