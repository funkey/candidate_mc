#ifndef CANDIDATE_MC_CRAG_CRAG_H__
#define CANDIDATE_MC_CRAG_CRAG_H__

#include <set>
#include <lemon/list_graph.h>
#define WITH_LEMON
#include <vigra/multi_gridgraph.hxx>
#include <util/exceptions.h>

/**
 * Candidate region adjacency graph.
 *
 * This data structure holds two graphs on the same set of nodes: An undirected
 * region adjacency graph (rag) and a directed subset graph (subset).
 *
 * Each node and adjacency edge has a type (which defaults to Volume and 
 * Adjacency, respectively) which can be used to specialize feature extraction 
 * and solvers.
 */
class Crag {

public:

	enum NodeType {

		/**
		 * Indicates that the candidate is a 3D region. Default.
		 */
		VolumeNode,

		/**
		 * Indicates that the candidate is a 2D region (possibly in a 3D 
		 * volume). This information is used to extract features that are 
		 * specific for 2D regions.
		 */
		SliceNode,

		/**
		 * Indicates that the candidate represents an assignment of slices 
		 * across sections of a volume. Slice and Assignment candidates are 
		 * supposed to form a bipartite graph on the adjacency edges.
		 */
		AssignmentNode,

		/**
		 * A special "assignment" node that represents no assignment of a 
		 * candidate. This node has no features and no costs.
		 */
		NoAssignmentNode
	};
	static const std::vector<NodeType> NodeTypes;

	enum EdgeType {

		/**
		 * Indicates that the connected candidates are adjacent.
		 */
		AdjacencyEdge,

		/**
		 * Indicates that the connected candidates have to belong to different 
		 * regions. To be used by solvers to enforce user constraints.
		 */
		SeparationEdge,

		/**
		 * An adjecency edge that links Slice nodes and Assignment nodes. This 
		 * edge has no features and no costs.
		 */
		AssignmentEdge,

		/**
		 * A special "assignment" edge connecting Slice nodes to the 
		 * NoAssignment node. This edge has features to model costs for the 
		 * appearance and disappearance of tracks.
		 */
		NoAssignmentEdge
	};
	static const std::vector<EdgeType> EdgeTypes;

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

	class CragArc;
	class CragEdge;
	class CragNodeIterator;
	class CragIncEdgeIterator;
	class CragIncEdges;
	template <typename T>
	class CragIncArcs;

	struct InArcTag  {

		typedef SubsetType::InArcIt IteratorType;
	};

	struct OutArcTag {

		typedef SubsetType::OutArcIt IteratorType;
	};

	class CragNode {

		friend class Crag;
		friend class CragArc;
		friend class CragEdge;
		friend class CragNodeIterator;
		friend class CragIncEdgeIterator;
		friend class CragIncEdges;
		template <typename T>
		friend class CragIncArcs;

		RagType::Node _node;

	public:

		CragNode(RagType::Node n)
			: _node(n) {}

		/**
		 * Create an uninitialized node.
		 */
		CragNode() : _node(lemon::INVALID) {}

		/**
		 * Strict ordering on nodes.
		 */
		bool operator<(CragNode other) const {

			return _node < other._node;
		}

		bool operator==(CragNode other) const {

			return _node == other._node;
		}

		bool operator!=(CragNode other) const {

			return !(_node == other._node);
		}

		/**
		 * Implicit conversion operator to a node of the lemon region adjacency 
		 * graph. Provided for convenience, such that this node can be used as 
		 * the key in a lemon node map and for the underlying lemon graph 
		 * RagType.
		 */
		operator RagType::Node () const {

			return _node;
		}
	};

	class CragArc {

		const Crag& _crag;
		SubsetType::Arc _arc;

	public:

		CragArc(const Crag& g, SubsetType::Arc a)
			: _crag(g), _arc(a) {}

		CragNode source() {

			return CragNode(_crag.toRag(_crag.getSubsetGraph().source(_arc)));
		}

		CragNode target() {

			return CragNode(_crag.toRag(_crag.getSubsetGraph().target(_arc)));
		}

		/**
		 * Implicit conversion operator to an arc of the lemon subset graph. 
		 * Provided for convenience, such that this arc can be used as the key 
		 * in a lemon arc map and for the underlying lemon graph SubsetType.
		 */
		operator SubsetType::Arc () const {

			return _arc;
		}
	};

	class CragEdge {

		const Crag& _crag;
		RagType::Edge _edge;

	public:

		CragEdge(const Crag& g, RagType::Edge e)
			: _crag(g), _edge(e) {}

		CragNode u() const {

			return CragNode(_crag.getAdjacencyGraph().u(_edge));
		}

		CragNode v() const {

			return CragNode(_crag.getAdjacencyGraph().v(_edge));
		}

		CragNode opposite(CragNode n) {

			return CragNode(_crag.getAdjacencyGraph().oppositeNode(n, _edge));
		}

		bool operator<(const CragEdge& other) const {

			return _edge < other._edge;
		}

		/**
		 * Implicit conversion operator to an edge of the lemon region adjacency 
		 * graph. Provided for convenience, such that this edge can be used as 
		 * the key in a lemon edge map and for the underlying lemon graph 
		 * RagType.
		 */
		operator RagType::Edge () const {

			return _edge;
		}
	};

	#include "CragIterators.h"

	Crag() :
		_nodeTypes(_rag),
		_edgeTypes(_rag),
		_affiliatedEdges(_rag) {}

	virtual ~Crag() {}

	/**
	 * Add a node to the CRAG.
	 */
	inline CragNode addNode(NodeType type) {

		_ssg.addNode();
		CragNode n = _rag.addNode();
		_nodeTypes[n] = type;

		return n;
	}
	inline CragNode addNode() { return addNode(VolumeNode); }

	/**
	 * Remove a node an its adjacency edges and subset arcs.
	 */
	inline void erase(Crag::CragNode n) {

		_ssg.erase(toSubset(n));
		_rag.erase(n);
	}

	/**
	 * Remove an adjacency edge.
	 */
	inline void erase(Crag::CragEdge e) {

		_rag.erase(e);
	}

	/**
	 * Remove a subset arc.
	 */
	inline void erase(Crag::CragArc a) {

		_ssg.erase(a);
	}

	/**
	 * Indicate that the candidates represented by the given two nodes are 
	 * adjacent.
	 */
	inline CragEdge addAdjacencyEdge(CragNode u, CragNode v, EdgeType type) {

		CragEdge e(*this, _rag.addEdge(u, v));
		_edgeTypes[e] = type;

		return e;
	}
	inline CragEdge addAdjacencyEdge(CragNode u, CragNode v) { return addAdjacencyEdge(u, v, AdjacencyEdge); }

	/**
	 * Indicate that the candidate represented by node u is a subset of the 
	 * candidate represented by node v.
	 */
	inline CragArc addSubsetArc(CragNode u, CragNode v) {

		return CragArc(*this, _ssg.addArc(toSubset(u), toSubset(v)));
	}

	CragNodes nodes() const { return CragNodes(*this); }

	CragEdges edges() const { return CragEdges(*this); }

	CragArcs  arcs()  const { return CragArcs (*this); }

	/**
	 * Get all outgoing subset arcs of a node, i.e., arcs to supernodes.
	 */
	CragIncArcs<OutArcTag> outArcs(CragNode n) const {

		return CragIncArcs<OutArcTag>(*this, n);
	}

	/**
	 * Get all incoming subset arcs of a node, i.e., arcs to subnodes.
	 */
	CragIncArcs<InArcTag> inArcs(CragNode n) const {

		return CragIncArcs<InArcTag>(*this, n);
	}

	/**
	 * Get all adjacency edges of a node.
	 */
	CragIncEdges adjEdges(CragNode n) const {

		return CragIncEdges(*this, n);
	}

	/**
	 * Get the type of a node.
	 */
	NodeType type(CragNode n) const { return _nodeTypes[n]; }

	/**
	 * Get the type of an edge.
	 */
	EdgeType type(CragEdge e) const { return _edgeTypes[e]; }

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
	void setAffiliatedEdges(CragEdge e, const std::vector<vigra::GridGraph<3>::Edge>& edges) {

		if (!isLeafEdge(e))
			UTIL_THROW_EXCEPTION(UsageError, "affiliated edges can only be set for leaf edges");
		 _affiliatedEdges[e] = edges;
	}

	/**
	 * Get affiliated edges for a leaf edge.
	 */
	const std::vector<vigra::GridGraph<3>::Edge>& getAffiliatedEdges(CragEdge e) const {

		if (!isLeafEdge(e))
			UTIL_THROW_EXCEPTION(UsageError, "affiliated edges only set for leaf edges");
		return _affiliatedEdges[e];
	}

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
	int getLevel(Crag::CragNode n) const;

	/**
	 * Return true for candidates that are leaf nodes in the subset graph.
	 */
	bool isLeafNode(Crag::CragNode n) const { return (SubsetInArcIt(*this, toSubset(n)) == lemon::INVALID); }

	/**
	 * Return true for candidates that are root nodes in the subset graph.
	 */
	bool isRootNode(Crag::CragNode n) const { return (SubsetOutArcIt(*this, toSubset(n)) == lemon::INVALID); }

	/**
	 * Return true for edges that connect two leaf nodes.
	 */
	bool isLeafEdge(Crag::CragEdge e) const { return (isLeafNode(e.u()) && isLeafNode(e.v())); }

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

	inline int id(CragNode n) const { return _rag.id(n); }
	inline int id(CragEdge e) const { return _rag.id(e); }
	inline int id(CragArc  a) const { return _ssg.id(a); }

	/**
	 * Convenience function to create a node from an id.
	 */
	inline CragNode nodeFromId(int id) const {

		return CragNode(_rag.nodeFromId(id));
	}

	/**
	 * Get the opposite node of an adjacency edge.
	 */
	inline CragNode oppositeNode(CragNode n, CragEdge e) const {

		return _rag.oppositeNode(n, e);
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

	inline size_t numNodes() const {

		size_t s = 0;
		for (auto i = nodes().begin(); i != nodes().end(); i++, s++) {}
		return s;
	}

	inline size_t numEdges() const {

		size_t s = 0;
		for (auto i = edges().begin(); i != edges().end(); i++, s++) {}
		return s;
	}

	inline size_t numArcs() const {

		size_t s = 0;
		for (auto i = arcs().begin(); i != arcs().end(); i++, s++) {}
		return s;
	}

	static CragNode Invalid;

	NodeMap<NodeType>& nodeTypes() { return _nodeTypes; }
	const NodeMap<NodeType>& nodeTypes() const { return _nodeTypes; }
	EdgeMap<EdgeType>& edgeTypes() { return _edgeTypes; }
	const EdgeMap<EdgeType>& edgeTypes() const { return _edgeTypes; }

	/**
	 * Get all leaf nodes under the given node n.
	 */
	std::set<CragNode> leafNodes(CragNode n) const;

	/**
	 * Get all leaf edges under the given node n.
	 */
	std::set<CragEdge> leafEdges(CragNode n) const;

	/**
	 * Get all leaf edges under the given edge e.
	 */
	std::set<CragEdge> leafEdges(CragEdge e) const;

	/**
	 * Get all edges that are descendants of e. These are all edges that are 
	 * linking descendants of the nodes connected by e.
	 */
	std::set<CragEdge> descendantEdges(CragEdge e) const;

	/**
	 * Get all edges that are linking descendants of u and v.
	 */
	std::set<CragEdge> descendantEdges(CragNode u, CragNode v) const;

private:

	void recLeafNodes(CragNode n, std::set<CragNode>& leafNodes) const;

	void recCollectEdges(Crag::CragNode n, std::set<Crag::CragEdge>& edges) const;

	// adjacency graph
	lemon::ListGraph _rag;

	// subset graph
	lemon::ListDigraph _ssg;

	NodeMap<NodeType> _nodeTypes;

	EdgeMap<EdgeType> _edgeTypes;

	vigra::GridGraph<3> _gridGraph;

	// voxel edges between adjacent leaf nodes
	EdgeMap<std::vector<vigra::GridGraph<3>::Edge>> _affiliatedEdges;
};

#endif // CANDIDATE_MC_CRAG_CRAG_H__

