#include <util/assert.h>
#include "Hdf5DigraphReader.h"

void
Hdf5DigraphReader::readDigraph(Digraph& digraph) {

	if (!_hdfFile.existsDataset("num_nodes"))
		return;

	vigra::ArrayVector<int> nodes;
	vigra::ArrayVector<int> arcs; // stored in pairs

	_hdfFile.readAndResize(
			"num_nodes",
			nodes);
	int numNodes = nodes[0];

	if (numNodes == 0)
		return;

	if (_hdfFile.existsDataset("arcs"))
		_hdfFile.readAndResize(
				"arcs",
				arcs);

	for (int i = 0; i < numNodes; i++) {

		Digraph::Node node = digraph.addNode();
		UTIL_ASSERT_REL(digraph.id(node), ==, i);
	}

	for (unsigned int i = 0; i < arcs.size(); i += 2) {

		Digraph::Node u = digraph.nodeFromId(arcs[i]);
		Digraph::Node v = digraph.nodeFromId(arcs[i+1]);

		digraph.addArc(u, v);
	}
}
