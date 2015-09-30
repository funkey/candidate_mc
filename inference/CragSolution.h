#ifndef CANDIDATE_MC_INFERENCE_CRAG_SOLUTION_H__
#define CANDIDATE_MC_INFERENCE_CRAG_SOLUTION_H__

#include <crag/Crag.h>

/**
 * Represents a CRAG solution in terms of selected nodes and edges, and as a 
 * connected component labeling of the selected nodes.
 */
class CragSolution {

public:

	CragSolution(const Crag& crag) :
		_crag(crag),
		_selectedNodes(crag),
		_selectedEdges(crag),
		_labels(crag),
		_labelsDirty(true) {}

	void setSelected(Crag::CragNode n, bool selected) { _selectedNodes[n] = selected; _labelsDirty = true; }

	void setSelected(Crag::CragEdge e, bool selected) { _selectedEdges[e] = selected; _labelsDirty = true; }

	inline bool selected(Crag::CragNode n) const { return _selectedNodes[n]; }

	inline bool selected(Crag::CragEdge e) const { return _selectedEdges[e]; }

	/**
	 * Get the id of the connected component the given candidate belongs to. 
	 * Returns 0, if the candidate was not selected.
	 */
	inline int label(Crag::CragNode n) const {

		if (_labelsDirty) {

			findConnectedComponents();
			_labelsDirty = false;
		}

		return _labels[n];
	}

private:

	void findConnectedComponents() const;

	const Crag& _crag;

	Crag::NodeMap<bool> _selectedNodes;
	Crag::EdgeMap<bool> _selectedEdges;

	mutable Crag::NodeMap<int>  _labels;
	mutable bool _labelsDirty;
};

#endif // CANDIDATE_MC_INFERENCE_CRAG_SOLUTION_H__

