#include <tests.h>

BEGIN_TEST_SUITE(crag)

	ADD_TEST_CASE(create_crag)
	ADD_TEST_CASE(modify_crag)
	ADD_TEST_CASE(hdf5_store)
	ADD_TEST_CASE(crag_iterators)
	ADD_TEST_CASE(volumes)

END_TEST_SUITE()
