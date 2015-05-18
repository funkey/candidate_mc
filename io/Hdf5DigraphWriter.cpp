#include "Hdf5DigraphWriter.h"

void
Hdf5DigraphWriter::writeDigraph(const Hdf5DigraphWriter::Digraph& digraph) {

	int numNodes = 0;
	for (Digraph::NodeIt node(digraph); node != lemon::INVALID; ++node)
		numNodes++;

	vigra::ArrayVector<int> n(1);
	vigra::ArrayVector<int> arcs; // stored in pairs

	n[0] = numNodes;
	_hdfFile.write("num_nodes", n);

	if (!nodeIdsConsequtive(digraph)) {

		// create map from node ids to consecutive ids
		std::map<int, int> nodeMap = createNodeMap(digraph);

		for (Digraph::ArcIt arc(digraph); arc != lemon::INVALID; ++arc) {

			arcs.push_back(nodeMap[digraph.id(digraph.source(arc))]);
			arcs.push_back(nodeMap[digraph.id(digraph.target(arc))]);
		}

	} else {

		for (Digraph::ArcIt arc(digraph); arc != lemon::INVALID; ++arc) {

			arcs.push_back(digraph.id(digraph.source(arc)));
			arcs.push_back(digraph.id(digraph.target(arc)));
		}
	}

	_hdfFile.write("arcs", arcs);
}

bool
Hdf5DigraphWriter::nodeIdsConsequtive(const Hdf5DigraphWriter::Digraph& digraph) {

	int i = -1;
	bool ascending;

	for (Digraph::NodeIt node(digraph); node != lemon::INVALID; ++node) {

		if (i == -1) {

			if (digraph.id(node) != 0) {

				i = digraph.id(node);
				ascending = false;

			} else {

				i = 0;
				ascending = true;
			}
		}

		if (digraph.id(node) != i)
			return false;

		if (ascending)
			i++;
		else
			i--;
	}

	return true;
}

std::map<int, int>
Hdf5DigraphWriter::createNodeMap(const Hdf5DigraphWriter::Digraph& digraph) {

	std::map<int, int> nodeMap;
	int i = 0;
	for (Digraph::NodeIt node(digraph); node != lemon::INVALID; ++node) {

		nodeMap[digraph.id(node)] = i;
		i++;
	}

	return nodeMap;
}
