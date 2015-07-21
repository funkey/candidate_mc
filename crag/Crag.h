#ifndef CANDIDATE_MC_CRAG_CRAG_H__
#define CANDIDATE_MC_CRAG_CRAG_H__

#include <lemon/list_graph.h>
#define WITH_LEMON
#include <vigra/multi_gridgraph.hxx>

/**
 * Candidate region adjacency graph.
 *
 * This datastructure holds two graphs on the same set of nodes: An undirected 
 * region adjacency graph (rag) and a directed subset graph (subset).
 */
class Crag {

public:

	typedef lemon::ListGraph     RagType;
	typedef lemon::ListDigraph   SubsetType;

	typedef RagType::Node        Node;
	typedef RagType::NodeIt      NodeIt;
	typedef RagType::Edge        Edge;
	typedef RagType::EdgeIt      EdgeIt;
	typedef RagType::IncEdgeIt   IncEdgeIt;

	typedef SubsetType::Node     SubsetNode;
	typedef SubsetType::NodeIt   SubsetNodeIt;
	typedef SubsetType::Arc      SubsetArc;
	typedef SubsetType::ArcIt    SubsetArcIt;
	typedef SubsetType::OutArcIt SubsetOutArcIt;
	typedef SubsetType::InArcIt  SubsetInArcIt;

	template <typename T> using NodeMap = RagType::NodeMap<T>;
	template <typename T> using EdgeMap = RagType::EdgeMap<T>;

	Crag() :
		_affiliatedEdges(_rag) {}

	virtual ~Crag() {}

	/**
	 * Add a node to the CRAG.
	 */
	inline Node addNode() {

		_ssg.addNode();
		return _rag.addNode();
	}

	/**
	 * Remove a node an its adjacency edges and subset arcs.
	 */
	inline void erase(Crag::Node n) {

		_ssg.erase(toSubset(n));
		_rag.erase(n);
	}
	inline void erase(Crag::SubsetNode n) {

		_ssg.erase(n);
		_rag.erase(toRag(n));
	}

	/**
	 * Remove an adjacency edge.
	 */
	inline void erase(Crag::Edge e) {

		_rag.erase(e);
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
	 * Set the grid graph, to which the affiliated edges between leaf node 
	 * regions refer.
	 */
	void setGridGraph(const vigra::GridGraph<3>& gridGraph) {

		_gridGraph = gridGraph;
	}

	/**
	 * Associate affiliated edges to a pair of adjacent leaf node regions. It is 
	 * assumed that an adjacency edge has already been added between u and v.
	 */
	void setAffiliatedEdges(Edge e, const std::vector<vigra::GridGraph<3>::Edge>& edges) { _affiliatedEdges[e] = edges; }

	const std::vector<vigra::GridGraph<3>::Edge>& getAffiliatedEdges(Edge e) const { return _affiliatedEdges[e]; }

	const vigra::GridGraph<3>& getGridGraph() const { return _gridGraph; }

	/**
	 * Get direct access to the underlying lemon graphs.
	 */
	const lemon::ListGraph&   getAdjacencyGraph() const { return _rag; }
	      lemon::ListGraph&   getAdjacencyGraph()       { return _rag; }
	const lemon::ListDigraph& getSubsetGraph()    const { return _ssg; }
	      lemon::ListDigraph& getSubsetGraph()          { return _ssg; }

	/**
	 * Get the level of a node, i.e., the size of the longest subset-tree path 
	 * to a leaf node. Leaf nodes have a value of zero.
	 */
	int getLevel(Crag::Node n) const;

	/**
	 * Return true for candidates that are leaf nodes in the subset graph.
	 */
	bool isLeafNode(Crag::Node n) const { return (SubsetInArcIt(*this, toSubset(n)) == lemon::INVALID); }

	/**
	 * Return true for candidates that are root nodes in the subset graph.
	 */
	bool isRootNode(Crag::Node n) const { return (SubsetOutArcIt(*this, toSubset(n)) == lemon::INVALID); }

	/**
	 * Implicit conversion operators for iteratos, node-, edge-, and arc-map 
	 * creation.
	 */
	operator const RagType& ()    const { return _rag; }
	operator       RagType& ()          { return _rag; }
	operator const SubsetType& () const { return _ssg; }
	operator       SubsetType& ()       { return _ssg; }

	inline Node u(Edge e) const { return _rag.u(e); }
	inline Node v(Edge e) const { return _rag.v(e); }

	inline int id(Node n)       const { return _rag.id(n); }
	inline int id(SubsetNode n) const { return _ssg.id(n); }
	inline int id(Edge e)       const { return _rag.id(e); }
	inline int id(SubsetArc  a) const { return _ssg.id(a); }

	/**
	 * Convenience function to create a node from an id.
	 */
	inline Node nodeFromId(int id) const {

		return _rag.nodeFromId(id);
	}

	/**
	 * Convert a subset node into a rag node.
	 */
	inline Node toRag(SubsetNode n) const {

		return _rag.nodeFromId(_ssg.id(n));
	}

	/**
	 * Convert a rag node into a subset node.
	 */
	inline SubsetNode toSubset(Node n) const {

		return _ssg.nodeFromId(_rag.id(n));
	}

private:

	// adjacency graph
	lemon::ListGraph _rag;

	// subset graph
	lemon::ListDigraph _ssg;

	vigra::GridGraph<3> _gridGraph;

	// voxel edges between adjacent leaf nodes
	EdgeMap<std::vector<vigra::GridGraph<3>::Edge>> _affiliatedEdges;
};

#endif // CANDIDATE_MC_CRAG_CRAG_H__

