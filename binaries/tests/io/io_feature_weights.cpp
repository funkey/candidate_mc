#include <tests.h>
#include <features/FeatureWeights.h>
#include <io/Hdf5CragStore.h>

void io_feature_weights() {

	FeatureWeights weights;

	// popluate feature weights
	weights[Crag::VolumeNode]       = {0};
	weights[Crag::SliceNode]        = {10, 11};
	weights[Crag::AssignmentNode]   = {20, 21, 22};
	weights[Crag::AdjacencyEdge]    = {30, 31, 32, 33};
	weights[Crag::NoAssignmentEdge] = {40, 41, 42, 43, 44};

	// save
	Hdf5CragStore store("io_test.hdf");
	store.saveFeatureWeights(weights);
	store.retrieveFeatureWeights(weights);

	for (int i = 0; i < 1; i++)
		BOOST_CHECK_EQUAL(weights[Crag::VolumeNode][i], i);
	for (int i = 0; i < 2; i++)
		BOOST_CHECK_EQUAL(weights[Crag::SliceNode][i], 10 + i);
	for (int i = 0; i < 3; i++)
		BOOST_CHECK_EQUAL(weights[Crag::AssignmentNode][i], 20 + i);
	for (int i = 0; i < 4; i++)
		BOOST_CHECK_EQUAL(weights[Crag::AdjacencyEdge][i], 30 + i);
	for (int i = 0; i < 5; i++)
		BOOST_CHECK_EQUAL(weights[Crag::NoAssignmentEdge][i], 40 + i);
}
