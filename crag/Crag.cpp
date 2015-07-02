#include "Crag.h"
#include <util/assert.h>

util::box<float, 3>
Crag::getBoundingBox(Crag::Node n) const {

	util::box<float, 3> bb;

	int numChildren = 0;
	for (Crag::SubsetInArcIt e(*this, toSubset(n)); e != lemon::INVALID; ++e) {

		bb += getBoundingBox(toRag(getSubsetGraph().oppositeNode(toSubset(n), e)));
		numChildren++;
	}

	if (numChildren == 0) {

		// leaf node
		return _volumes[n].getBoundingBox();
	}

	return bb;
}

const ExplicitVolume<unsigned char>&
Crag::getVolume(Crag::Node n) const {

	if (_volumes[n].getBoundingBox().isZero()) {

		const util::box<float, 3>& nodeBoundingBox = getBoundingBox(n);
		recFill(nodeBoundingBox, _volumes[n], n);

		UTIL_ASSERT_REL(nodeBoundingBox, ==, _volumes[n].getBoundingBox());
	}

	return _volumes[n];
}

ExplicitVolume<unsigned char>&
Crag::getVolume(Crag::Node n) {

	if (_volumes[n].getBoundingBox().isZero()) {

		const util::box<float, 3>& nodeBoundingBox = getBoundingBox(n);
		recFill(nodeBoundingBox, _volumes[n], n);

		UTIL_ASSERT_REL(nodeBoundingBox, ==, _volumes[n].getBoundingBox());
	}

	return _volumes[n];
}

int
Crag::getLevel(Crag::Node n) const {

	if (SubsetInArcIt(*this, toSubset(n)) == lemon::INVALID)
		return 0;

	int level = 0;
	for (SubsetInArcIt e(*this, toSubset(n)); e != lemon::INVALID; ++e) {

		level = std::max(getLevel(toRag(getSubsetGraph().oppositeNode(toSubset(n), e))), level);
	}

	return level + 1;
}

void
Crag::recFill(
		const util::box<float, 3>&     boundingBox,
		ExplicitVolume<unsigned char>& volume,
		Crag::Node                     n) const {

	int numChildren = 0;
	for (Crag::SubsetInArcIt e(*this, toSubset(n)); e != lemon::INVALID; ++e) {

		recFill(boundingBox, volume, toRag(getSubsetGraph().oppositeNode(toSubset(n), e)));
		numChildren++;
	}

	if (numChildren == 0) {

		const ExplicitVolume<unsigned char>& leaf = _volumes[n];

		UTIL_ASSERT(!leaf.getBoundingBox().isZero());

		util::point<unsigned int, 3> volumeOffset =
				boundingBox.min()/
				leaf.getResolution();
		util::point<unsigned int, 3> leafOffset =
				leaf.getBoundingBox().min()/
				leaf.getResolution();
		util::point<unsigned int, 3> offset = leafOffset - volumeOffset;

		if (volume.getBoundingBox().isZero()) {

			volume = ExplicitVolume<unsigned char>(
					boundingBox.width() /leaf.getResolution().x(),
					boundingBox.height()/leaf.getResolution().y(),
					boundingBox.depth() /leaf.getResolution().z());
			volume.setOffset(boundingBox.min());
			volume.setResolution(leaf.getResolution());
		}

		for (unsigned int z = 0; z < leaf.depth();  z++)
		for (unsigned int y = 0; y < leaf.height(); y++)
		for (unsigned int x = 0; x < leaf.width();  x++) {

			if (leaf(x, y, z) > 0)
				volume(
						offset.x() + x,
						offset.y() + y,
						offset.z() + z) = leaf(x, y, z);
		}
	}
}

