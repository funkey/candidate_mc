#include "Overlap.h"
#include <util/assert.h>

double
Overlap::operator()(const CragVolume& a, const CragVolume& b) {

	UTIL_ASSERT_REL(a.getResolution(), ==, b.getResolution());

	double overlap = 0;
	double voxelVolume = a.getResolution().x()*a.getResolution().y()*a.getResolution().z();

	util::point<int, 3> offset = (b.getOffset() - a.getOffset())/a.getResolution();

	for (int z = 0; z < a.depth();  z++)
	for (int y = 0; y < a.height(); y++)
	for (int x = 0; x < a.width();  x++) {

		if (a(x, y, z) == 0)
			continue;

		util::point<int, 3> bpos(
				x + offset.x(),
				y + offset.y(),
				z + offset.z());

		if (!b.getDiscreteBoundingBox().contains(bpos))
			continue;

		if (b[bpos] > 0)
			overlap += voxelVolume;
	}

	return overlap;
}

bool
Overlap::exceeds(const CragVolume& a, CragVolume& b, double value) {

	if (a.getBoundingBox().intersection(b.getBoundingBox()).volume() <= value)
		return false;

	return (*this)(a, b) > value;
}
