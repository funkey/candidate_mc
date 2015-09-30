#include <lemon/connectivity.h>
#include "CragSolution.h"

void
CragSolution::findConnectedComponents() const {

	typedef lemon::ListGraph Cut;
	Cut labelGraph;
	for (unsigned int i = 0; i < _crag.nodes().size(); i++)
		labelGraph.addNode();

	for (Crag::CragEdge e : _crag.edges())
		if (_selectedEdges[e])
			labelGraph.addEdge(e.u(), e.v());

	// find connected components in cut graph
	lemon::connectedComponents(labelGraph, _labels);
}
