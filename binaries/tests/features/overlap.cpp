#include <map>
#include <tests.h>
#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <features/Overlap.h>

void overlap() {

	Overlap overlap;

	CragVolume a(10, 10, 1);
	CragVolume b(10, 10, 1);

	BOOST_CHECK_EQUAL(overlap(a, b), 0);
	BOOST_CHECK_EQUAL(overlap(b, a), 0);
	BOOST_CHECK(!overlap.exceeds(b, a, 0));

	a(0, 0, 0) = 1;
	b.data() = 1;

	BOOST_CHECK_EQUAL(overlap(a, b), 1);
	BOOST_CHECK_EQUAL(overlap(b, a), 1);
	BOOST_CHECK(overlap.exceeds(b, a, 0));
	BOOST_CHECK(!overlap.exceeds(b, a, 1));

	a.setResolution(1, 2, 1);

	BOOST_CHECK_EQUAL(overlap(a, b), 2);
	BOOST_CHECK_EQUAL(overlap(b, a), 2);
	BOOST_CHECK(overlap.exceeds(b, a, 0));
	BOOST_CHECK(overlap.exceeds(b, a, 1));
	BOOST_CHECK(!overlap.exceeds(b, a, 2));

	b(0, 0, 0) = 0;

	BOOST_CHECK_EQUAL(overlap(a, b), 0);
	BOOST_CHECK_EQUAL(overlap(b, a), 0);
	BOOST_CHECK(!overlap.exceeds(b, a, 0));
}
