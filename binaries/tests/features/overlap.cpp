#include <map>
#include <tests.h>
#include <imageprocessing/intersect.h>
#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <features/Overlap.h>

void overlap() {

	Overlap overlap;

	CragVolume a(10, 10, 1);
	CragVolume b(10, 10, 1);
	CragVolume c;

	BOOST_CHECK_EQUAL(overlap(a, b), 0);
	BOOST_CHECK_EQUAL(overlap(b, a), 0);
	BOOST_CHECK(!overlap.exceeds(b, a, 0));

	intersect(a, b, c);

	BOOST_CHECK_EQUAL(c.getDiscreteBoundingBox().volume(), 0);

	a(0, 0, 0) = 1;
	b.data() = 1;

	BOOST_CHECK_EQUAL(overlap(a, b), 1);
	BOOST_CHECK_EQUAL(overlap(b, a), 1);
	BOOST_CHECK(overlap.exceeds(b, a, 0));
	BOOST_CHECK(!overlap.exceeds(b, a, 1));

	intersect(a, b, c);

	BOOST_CHECK_EQUAL(c.getDiscreteBoundingBox().volume(), 1);
	BOOST_CHECK_EQUAL(c(0, 0, 0), 1);

	a.setResolution(1, 2, 1);
	b.setResolution(1, 2, 1);

	BOOST_CHECK_EQUAL(overlap(a, b), 2);
	BOOST_CHECK_EQUAL(overlap(b, a), 2);
	BOOST_CHECK(overlap.exceeds(b, a, 0));
	BOOST_CHECK(overlap.exceeds(b, a, 1));
	BOOST_CHECK(!overlap.exceeds(b, a, 2));

	intersect(a, b, c);

	BOOST_CHECK_EQUAL(c.getDiscreteBoundingBox().volume(), 1);
	BOOST_CHECK_EQUAL(c.getBoundingBox().volume(), 2);

	a.setOffset(10, 10, 0);

	BOOST_CHECK_EQUAL(overlap(a, b), 0);
	BOOST_CHECK_EQUAL(overlap(b, a), 0);

	a.setOffset(9, 9, 0);

	BOOST_CHECK_EQUAL(overlap(a, b), 2);
	BOOST_CHECK_EQUAL(overlap(b, a), 2);

	a.setOffset(9, 9, 1);

	BOOST_CHECK_EQUAL(overlap(a, b), 0);
	BOOST_CHECK_EQUAL(overlap(b, a), 0);

	b(0, 0, 0) = 0;

	BOOST_CHECK_EQUAL(overlap(a, b), 0);
	BOOST_CHECK_EQUAL(overlap(b, a), 0);
	BOOST_CHECK(!overlap.exceeds(b, a, 0));
}
