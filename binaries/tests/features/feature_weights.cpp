#include <tests.h>
#include <features/FeatureWeights.h>

void feature_weights() {

	FeatureWeights weights;
	std::vector<double> w;

	// test empty weights
	weights.importFromVector(w);
	w = weights.exportToVector();

	BOOST_CHECK_EQUAL(w.size(), 0);

	// popluate feature weights
	weights[Crag::VolumeNode] = {0, 1, 2, 3, 4, 5};

	// test export
	w = weights.exportToVector();
	for (int i = 0; i < 6; i++)
		BOOST_CHECK_EQUAL(w[i], i);

	// test import
	w = {5, 4, 3, 2, 1, 0};
	weights.importFromVector(w);
	for (int i = 0; i < 6; i++)
		BOOST_CHECK_EQUAL(weights[Crag::VolumeNode][5-i], i);

	// add feature weights of different length for other types
	weights[Crag::SliceNode]        = {10, 11};
	weights[Crag::AssignmentNode]   = {20, 21, 22};
	weights[Crag::AdjacencyEdge]    = {30, 31, 32, 33};
	weights[Crag::NoAssignmentEdge] = {40, 41, 42, 43, 44};

	w = weights.exportToVector();
	weights.importFromVector(w);

	for (int i = 0; i < 2; i++)
		BOOST_CHECK_EQUAL(weights[Crag::SliceNode][i], 10 + i);
	for (int i = 0; i < 3; i++)
		BOOST_CHECK_EQUAL(weights[Crag::AssignmentNode][i], 20 + i);
	for (int i = 0; i < 4; i++)
		BOOST_CHECK_EQUAL(weights[Crag::AdjacencyEdge][i], 30 + i);
	for (int i = 0; i < 5; i++)
		BOOST_CHECK_EQUAL(weights[Crag::NoAssignmentEdge][i], 40 + i);
}
