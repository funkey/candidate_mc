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

	//////////////////////////////////////////////////////////

	template <typename T>
	class CragIncArcs;
	class CragIncEdges;

	struct InArcTag  {

		typedef SubsetType::InArcIt IteratorType;
	};

	struct OutArcTag {

		typedef SubsetType::OutArcIt IteratorType;
	};

	class CragNode {

		template <typename T>
		friend class CragIncArcs;
		friend class CragIncEdges;

		const Crag& _crag;
		RagType::Node _node;

	public:

		CragNode(const Crag& g, RagType::Node n)
			: _crag(g), _node(n) {}

		CragIncArcs<OutArcTag> outarcs() {

			return CragIncArcs<OutArcTag>(*this);
		}

		CragIncArcs<InArcTag> inarcs() {

			return CragIncArcs<InArcTag>(*this);
		}

		CragIncEdges adjedges() {

			return CragIncEdges(*this);
		}

		operator RagType::Node () {

			return _node;
		}

		operator SubsetNode () {

			return _crag.toSubset(_node);
		}
	};

	class CragArc {

		const Crag& _crag;
		SubsetType::Arc _arc;

	public:

		CragArc(const Crag& g, SubsetType::Arc a)
			: _crag(g), _arc(a) {}

		CragNode source() {

			return CragNode(_crag, _crag.toRag(_crag.getSubsetGraph().source(_arc)));
		}

		CragNode target() {

			return CragNode(_crag, _crag.toRag(_crag.getSubsetGraph().target(_arc)));
		}
	};

	class CragEdge {

		const Crag& _crag;
		RagType::Edge _edge;

	public:

		CragEdge(const Crag& g, RagType::Edge e)
			: _crag(g), _edge(e) {}

		CragNode u() {

			return CragNode(_crag, _crag.getAdjacencyGraph().u(_edge));
		}

		CragNode v() {

			return CragNode(_crag, _crag.getAdjacencyGraph().v(_edge));
		}
	};

	class CragNodeIterator {

		const Crag& _crag;
		RagType::NodeIt _it;

	public:

		CragNodeIterator(const Crag& g)
			: _crag(g), _it(g.getAdjacencyGraph()) {}

		CragNodeIterator(const CragNodeIterator& i)
			: _crag(i._crag), _it(i._it) {}

		CragNodeIterator(const Crag& g, const lemon::Invalid& i)
			: _crag(g), _it(i) {}

		CragNodeIterator& operator++() {

			++_it;
			return *this;
		}

		CragNodeIterator operator++(int) {

			CragNodeIterator tmp(*this);
			operator++();
			return tmp;
		}

		bool operator==(const CragNodeIterator& rhs) {

			return _it == rhs._it;
		}

		bool operator!=(const CragNodeIterator& rhs) {

			return _it != rhs._it;
		}

		CragNode operator*() {

			return CragNode(_crag, _it);
		}
	};

	class CragEdgeIterator {

		const Crag& _crag;
		RagType::EdgeIt _it;

	public:

		CragEdgeIterator(const Crag& g)
			: _crag(g), _it(g.getAdjacencyGraph()) {}

		CragEdgeIterator(const CragEdgeIterator& i)
			: _crag(i._crag), _it(i._it) {}

		CragEdgeIterator(const Crag& g, const lemon::Invalid& i)
			: _crag(g), _it(i) {}

		CragEdgeIterator& operator++() {

			++_it;
			return *this;
		}

		CragEdgeIterator operator++(int) {

			CragEdgeIterator tmp(*this);
			operator++();
			return tmp;
		}

		bool operator==(const CragEdgeIterator& rhs) {

			return _it == rhs._it;
		}

		bool operator!=(const CragEdgeIterator& rhs) {

			return _it != rhs._it;
		}

		CragEdge operator*() {

			return CragEdge(_crag, _it);
		}
	};

	class CragArcIterator : public std::iterator<std::input_iterator_tag, CragArc> {

		const Crag& _crag;
		typename SubsetType::ArcIt _it; 

	public:

		CragArcIterator(const Crag& g)
			: _crag(g), _it(g.getSubsetGraph()) {}

		CragArcIterator(const CragArcIterator& i)
			: _crag(i._crag), _it(i._it) {}

		CragArcIterator(const Crag& g, const lemon::Invalid& i)
			: _crag(g), _it(i) {}

		CragArcIterator& operator++() {

			++_it;
			return *this;
		}

		CragArcIterator operator++(int) {

			CragArcIterator tmp(*this);
			operator++();
			return tmp;
		}

		bool operator==(const CragArcIterator& rhs) {

			return _it == rhs._it;
		}

		bool operator!=(const CragArcIterator& rhs) {

			return _it != rhs._it;
		}

		CragArc operator*() {

			return CragArc(_crag, _it);
		}
	};

	template <typename T>
	class CragIncArcIterator : public std::iterator<std::input_iterator_tag, CragArc> {

		const Crag& _crag;
		typename T::IteratorType _it; 

	public:

		CragIncArcIterator(const Crag& g, typename T::IteratorType i)
			: _crag(g), _it(i) {}

		CragIncArcIterator(const CragIncArcIterator<T>& i)
			: _crag(i._crag), _it(i._crag, i._it) {}

		CragIncArcIterator<T>& operator++() {

			++_it;
			return *this;
		}

		CragIncArcIterator<T> operator++(int) {

			CragIncArcIterator<T> tmp(*this);
			operator++();
			return tmp;
		}

		bool operator==(const CragIncArcIterator<T>& rhs) {

			return _it == rhs._it;
		}

		bool operator!=(const CragIncArcIterator<T>& rhs) {

			return _it != rhs._it;
		}

		CragArc operator*() {

			return CragArc(_crag, _it);
		}
	};

	class CragIncEdgeIterator : public std::iterator<std::input_iterator_tag, CragEdge> {

		const Crag& _crag;
		typename RagType::IncEdgeIt _it; 

	public:

		CragIncEdgeIterator(const Crag& g, RagType::IncEdgeIt i)
			: _crag(g), _it(i) {}

		CragIncEdgeIterator(const CragIncEdgeIterator& i)
			: _crag(i._crag), _it(i._crag, i._it) {}

		CragIncEdgeIterator& operator++() {

			++_it;
			return *this;
		}

		CragIncEdgeIterator operator++(int) {

			CragIncEdgeIterator tmp(*this);
			operator++();
			return tmp;
		}

		bool operator==(const CragIncEdgeIterator& rhs) {

			return _it == rhs._it;
		}

		bool operator!=(const CragIncEdgeIterator& rhs) {

			return _it != rhs._it;
		}

		CragEdge operator*() {

			return CragEdge(_crag, _it);
		}
	};

	class CragNodes {

		friend class Crag;

		const Crag& _crag;

		CragNodes(const Crag& g) : _crag(g) {}

	public:

		CragNodeIterator begin() {

			return CragNodeIterator(_crag);
		}

		CragNodeIterator end() {

			return CragNodeIterator(_crag, lemon::INVALID);
		}
	};

	class CragEdges {

		friend class Crag;

		const Crag& _crag;

		CragEdges(const Crag& g) : _crag(g) {}

	public:

		CragEdgeIterator begin() {

			return CragEdgeIterator(_crag);
		}

		CragEdgeIterator end() {

			return CragEdgeIterator(_crag, lemon::INVALID);
		}
	};

	class CragArcs {

		friend class Crag;

		const Crag& _crag;

		CragArcs(const Crag& g) : _crag(g) {}

	public:

		CragArcIterator begin() {

			return CragArcIterator(_crag);
		}

		CragArcIterator end() {

			return CragArcIterator(_crag, lemon::INVALID);
		}
	};

	template <typename T>
	class CragIncArcs {

		friend class CragNode;

		const CragNode& _n;

		CragIncArcs(const CragNode& n) : _n(n) {}

	public:

		CragIncArcIterator<T> begin() {

			return CragIncArcIterator<T>(_n._crag, typename T::IteratorType(_n._crag.getSubsetGraph(), _n._crag.toSubset(_n._node)));
		}

		CragIncArcIterator<T> end() {

			return CragIncArcIterator<T>(_n._crag, lemon::INVALID);
		}

	//	CragIncArcIterator<T> begin() const;
	//	CragIncArcIterator<T> cbegin() const;
	//	CragIncArcIterator<T> end();
	//	CragIncArcIterator<T> end() const;
	//	CragIncArcIterator<T> cend() const;
	};

	class CragIncEdges {

		friend class CragNode;

		const CragNode& _n;

		CragIncEdges(const CragNode& n) : _n(n) {}

	public:

		CragIncEdgeIterator begin() {

			return CragIncEdgeIterator(_n._crag, RagType::IncEdgeIt(_n._crag.getAdjacencyGraph(), _n._node));
		}

		CragIncEdgeIterator end() {

			return CragIncEdgeIterator(_n._crag, lemon::INVALID);
		}
	};

	//////////////////////////////////////////////////////////

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

	CragNodes nodes() { return CragNodes(*this); }

	CragEdges edges() { return CragEdges(*this); }

	CragArcs  arcs()  { return CragArcs (*this); }

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

