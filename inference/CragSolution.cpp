#include <lemon/connectivity.h>
#include "CragSolution.h"

void
CragSolution::findConnectedComponents() const {

	typedef lemon::ListGraph Cut;
	Cut labelGraph;

	// create a cut graph, i.e., a graph of only selected nodes and edges

	// add only selected nodes, remember mapping from original nodes to cut 
	// graph nodes
	Crag::NodeMap<Cut::Node> cutGraphNodes(_crag);
	for (unsigned int i = 0; i < _crag.nodes().size(); i++)
	for (Crag::CragNode n : _crag.nodes())
		if (_selectedNodes[n])
			cutGraphNodes[n] = labelGraph.addNode();

	// add selected edges
	for (Crag::CragEdge e : _crag.edges())
		if (_selectedEdges[e] && _crag.type(e) != Crag::NoAssignmentEdge)
			labelGraph.addEdge(
					cutGraphNodes[e.u()],
					cutGraphNodes[e.v()]);

	// find connected components in cut graph
	Cut::NodeMap<unsigned int> ccs(labelGraph);
	lemon::connectedComponents(labelGraph, ccs);

	// label original nodes
	for (Crag::CragNode n : _crag.nodes())
		if (_selectedNodes[n])
			_labels[n] = ccs[cutGraphNodes[n]] + 1;
		else
			_labels[n] = 0;
}
