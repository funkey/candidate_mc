/*
 * This file is meant to be included into the class definition of Crag.
 */

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

	CragNodeIterator begin() const {

		return CragNodeIterator(_crag);
	}

	CragNodeIterator end() const {

		return CragNodeIterator(_crag, lemon::INVALID);
	}

	CragNodeIterator cbegin() const {

		return begin();
	}

	CragNodeIterator cend() const {

		return end();
	}

	/**
	 * Get the number of nodes in the associated CRAG.
	 */
	size_t size() const {

		return _crag.numNodes();
	}
};

class CragEdges {

	friend class Crag;

	const Crag& _crag;

	CragEdges(const Crag& g) : _crag(g) {}

public:

	CragEdgeIterator begin() const {

		return CragEdgeIterator(_crag);
	}

	CragEdgeIterator end() const {

		return CragEdgeIterator(_crag, lemon::INVALID);
	}

	CragEdgeIterator cbegin() const {

		return begin();
	}

	CragEdgeIterator cend() const {

		return end();
	}

	size_t size() const {

		return _crag.numEdges();
	}
};

class CragArcs {

	friend class Crag;

	const Crag& _crag;

	CragArcs(const Crag& g) : _crag(g) {}

public:

	CragArcIterator begin() const {

		return CragArcIterator(_crag);
	}

	CragArcIterator end() const {

		return CragArcIterator(_crag, lemon::INVALID);
	}

	CragArcIterator cbegin() const {

		return begin();
	}

	CragArcIterator cend() const {

		return end();
	}

	size_t size() const {

		return _crag.numArcs();
	}
};

template <typename T>
class CragIncArcs {

	friend class CragNode;

	const CragNode& _n;

	CragIncArcs(const CragNode& n) : _n(n) {}

public:

	CragIncArcIterator<T> begin() const {

		return CragIncArcIterator<T>(_n._crag, typename T::IteratorType(_n._crag.getSubsetGraph(), _n._crag.toSubset(_n._node)));
	}

	CragIncArcIterator<T> end() const {

		return CragIncArcIterator<T>(_n._crag, lemon::INVALID);
	}

	CragIncArcIterator<T> cbegin() const {

		return begin();
	}

	CragIncArcIterator<T> cend() const {

		return end();
	}

	size_t size() const {

		size_t s = 0;
		for (auto i = begin(); i != end(); i++, s++) {}
		return s;
	}
};

class CragIncEdges {

	friend class CragNode;

	const CragNode& _n;

	CragIncEdges(const CragNode& n) : _n(n) {}

public:

	CragIncEdgeIterator begin() const {

		return CragIncEdgeIterator(_n._crag, RagType::IncEdgeIt(_n._crag.getAdjacencyGraph(), _n._node));
	}

	CragIncEdgeIterator end() const {

		return CragIncEdgeIterator(_n._crag, lemon::INVALID);
	}

	CragIncEdgeIterator cbegin() const {

		return begin();
	}

	CragIncEdgeIterator cend() const {

		return end();
	}

	size_t size() const {

		size_t s = 0;
		for (auto i = begin(); i != end(); i++, s++) {}
		return s;
	}
};

