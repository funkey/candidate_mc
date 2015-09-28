#include <tests.h>
#include <features/NodeFeatures.h>
#include <features/EdgeFeatures.h>

void features() {

	Crag crag;
	Crag::CragNode n1 = crag.addNode();
	Crag::CragNode n2 = crag.addNode();
	Crag::CragNode n3 = crag.addNode();
	Crag::CragEdge e12 = crag.addAdjacencyEdge(n1, n2);
	Crag::CragEdge e23 = crag.addAdjacencyEdge(n2, n3);

	{
		NodeFeatures features(crag);

		features.append(n1, 1);
		features.append(n2, 2);
		features.append(n3, 3);

		BOOST_CHECK_EQUAL(features.dims(Crag::VolumeNode), 1);
		BOOST_CHECK_EQUAL(features.dims(Crag::SliceNode), 0);
		BOOST_CHECK_EQUAL(features.dims(Crag::AssignmentNode), 0);

		features.append(n1, 1);
		features.append(n2, 2);
		features.append(n3, 3);

		BOOST_CHECK_EQUAL(features.dims(Crag::VolumeNode), 2);
		BOOST_CHECK_EQUAL(features.dims(Crag::SliceNode), 0);
		BOOST_CHECK_EQUAL(features.dims(Crag::AssignmentNode), 0);
	}

	{
		EdgeFeatures features(crag);

		features.append(e12, 1);
		features.append(e23, 2);

		BOOST_CHECK_EQUAL(features.dims(Crag::AdjacencyEdge), 1);
		BOOST_CHECK_EQUAL(features.dims(Crag::NoAssignmentEdge), 0);

		features.append(e12, 1);
		features.append(e23, 2);

		BOOST_CHECK_EQUAL(features.dims(Crag::AdjacencyEdge), 2);
		BOOST_CHECK_EQUAL(features.dims(Crag::NoAssignmentEdge), 0);
	}
}
