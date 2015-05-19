#ifndef CANDIDATE_MC_CRAG_CRAG_H__
#define CANDIDATE_MC_CRAG_CRAG_H__

#include <imageprocessing/ExplicitVolume.h>
#include <lemon/list_graph.h>

/**
 * Candidate region adjacency graph.
 *
 * This datastructure holds two graphs on the same set of nodes: An undirected 
 * region adjacency graph (rag) and a directed subset graph (subset). In 
 * addition to that, a volume property map is provided, which stores the voxels 
 * for each leaf candidate.
 */
class Crag : public Volume {

public:

	typedef lemon::ListGraph     RagType;
	typedef lemon::ListDigraph   SubsetType;

	typedef RagType::Node        Node;
	typedef RagType::NodeIt      NodeIt;
	typedef RagType::Edge        Edge;
	typedef RagType::EdgeIt      EdgeIt;
	typedef RagType::IncEdgeIt   IncEdgeIt;

	typedef SubsetType::Node     SubsetNode;
	typedef SubsetType::Arc      SubsetArc;
	typedef SubsetType::ArcIt    SubsetArcIt;
	typedef SubsetType::OutArcIt SubsetOutArcIt;
	typedef SubsetType::InArcIt  SubsetInArcIt;

	template <typename T> using NodeMap = RagType::NodeMap<T>;

	Crag() :
		_volumes(_rag) {}

	/**
	 * Add a node to the CRAG.
	 */
	inline Node addNode() {

		_ssg.addNode();
		return _rag.addNode();
	}

	/**
	 * Indicate that the candidates represented by the given two nodes are 
	 * adjacent.
	 */
	inline Edge addAdjacencyEdge(Node u, Node v) {

		return _rag.addEdge(u, v);
	}

	/**
	 * Indicate that the candidate represented by node u is a subset of the 
	 * candidate represented by node v.
	 */
	inline SubsetArc addSubsetArc(Node u, Node v) {

		return _ssg.addArc(toSubset(u), toSubset(v));
	}

	/**
	 * Get direct access to the underlying lemon graphs.
	 */
	const lemon::ListGraph&   getAdjacencyGraph() const { return _rag; }
	      lemon::ListGraph&   getAdjacencyGraph()       { return _rag; }
	const lemon::ListDigraph& getSubsetGraph()    const { return _ssg; }
	      lemon::ListDigraph& getSubsetGraph()          { return _ssg; }

	/**
	 * Get a node map for the volumes of the candidates. Only leaf candidates 
	 * will have a non-empty volume.
	 */
	const NodeMap<ExplicitVolume<unsigned char>>& getVolumes() const { return _volumes; }
	      NodeMap<ExplicitVolume<unsigned char>>& getVolumes()       { return _volumes; }

	/**
	 * Implicit conversion operators for iteratos, node-, edge-, and arc-map 
	 * creation.
	 */
	operator const RagType& ()    const { return _rag; }
	operator       RagType& ()          { return _rag; }
	operator const SubsetType& () const { return _ssg; }
	operator       SubsetType& ()       { return _ssg; }

	int id(Node n)    const { return _rag.id(n); }
	int id(SubsetNode n) const { return _ssg.id(n); }

	/**
	 * Convenience function to create a node from an id.
	 */
	inline Node nodeFromId(int id) const {

		return _rag.nodeFromId(id);
	}

	/**
	 * Convert a subset node into a rag node.
	 */
	inline Node toRag(SubsetNode n) {

		return _rag.nodeFromId(_ssg.id(n));
	}

	/**
	 * Convert a rag node into a subset node.
	 */
	inline SubsetNode toSubset(Node n) {

		return _ssg.nodeFromId(_rag.id(n));
	}

protected:

	util::box<float,3> computeBoundingBox() const override {

		util::box<float, 3> bb;
		for (NodeIt n(_rag); n != lemon::INVALID; ++n)
			bb += _volumes[n].getBoundingBox();

		return bb;
	}

private:

	// adjacency graph
	lemon::ListGraph _rag;

	// subset graph
	lemon::ListDigraph _ssg;

	// volumes of leaf candidates
	NodeMap<ExplicitVolume<unsigned char>> _volumes;
};

#endif // CANDIDATE_MC_CRAG_CRAG_H__

