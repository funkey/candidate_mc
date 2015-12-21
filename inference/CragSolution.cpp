#include <lemon/connectivity.h>
#include <util/Logger.h>
#include "CragSolution.h"

logger::LogChannel cragsolutionlog("cragsolutionlog", "[CragSolution] ");

void
CragSolution::findConnectedComponents() const {

	LOG_ALL(cragsolutionlog) << "searching for connected components" << std::endl;

	typedef lemon::ListGraph Cut;
	Cut labelGraph;

	// create a cut graph, i.e., a graph of only selected nodes and edges

	// add only selected nodes, remember mapping from original nodes to cut 
	// graph nodes
	Crag::NodeMap<Cut::Node> cutGraphNodes(_crag);
	LOG_ALL(cragsolutionlog) << "adding selected nodes" << std::endl;
	for (Crag::CragNode n : _crag.nodes())
		if (_selectedNodes[n])
			cutGraphNodes[n] = labelGraph.addNode();

	// add selected edges
	LOG_ALL(cragsolutionlog) << "adding selected edges" << std::endl;
	for (Crag::CragEdge e : _crag.edges())
		if (_selectedEdges[e] && _crag.type(e) != Crag::NoAssignmentEdge)
			labelGraph.addEdge(
					cutGraphNodes[e.u()],
					cutGraphNodes[e.v()]);

	// find connected components in cut graph
	Cut::NodeMap<unsigned int> ccs(labelGraph);
	LOG_ALL(cragsolutionlog) << "searching for connected components" << std::endl;
	lemon::connectedComponents(labelGraph, ccs);

	// label original nodes
	LOG_ALL(cragsolutionlog) << "reading labeling" << std::endl;
	for (Crag::CragNode n : _crag.nodes())
		if (_selectedNodes[n])
			_labels[n] = ccs[cutGraphNodes[n]] + 1;
		else
			_labels[n] = 0;

	LOG_ALL(cragsolutionlog) << "done" << std::endl;
}
