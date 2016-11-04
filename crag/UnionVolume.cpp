#include "UnionVolume.h"

std::shared_ptr<CragVolume>
UnionVolume::materialize() const {

	const util::box<unsigned int, 3>& dbb        = getDiscreteBoundingBox();
	const util::box<float, 3>&        bb         = getBoundingBox();
	const util::point<float, 3>&      resolution = getResolution();
	const util::point<float, 3>&      offset     = getOffset();

	// create a new volume
	auto m = std::make_shared<CragVolume>(
			dbb.width(),
			dbb.height(),
			dbb.depth());
	CragVolume& materialized = *m;
	materialized.setOffset(offset);
	materialized.setResolution(resolution);

	// descrete offset of materialized in global volume
	util::point<unsigned int, 3> materializedOffset = bb.min()/resolution;

	// combine union
	for (auto v : _union) {

		const CragVolume& volume = *v;

		// descrete offset of volume in global volume
		util::point<unsigned int, 3> volumeOffset =
				volume.getBoundingBox().min()/
				volume.getResolution();

		// offset to get from positions in materialized to positions in 
		// volume
		util::point<unsigned int, 3> offset = volumeOffset - materializedOffset;

		// copy volume into materialized
		for (unsigned int z = 0; z < volume.depth();  z++)
		for (unsigned int y = 0; y < volume.height(); y++)
		for (unsigned int x = 0; x < volume.width();  x++) {

			UTIL_ASSERT_REL(offset.x() + x, <, dbb.width());
			UTIL_ASSERT_REL(offset.y() + y, <, dbb.height());
			UTIL_ASSERT_REL(offset.z() + z, <, dbb.depth());

			if (volume(x, y, z) > 0)
				materialized(
						offset.x() + x,
						offset.y() + y,
						offset.z() + z) = volume(x, y, z);
		}
	}

	UTIL_ASSERT_REL(bb, ==, materialized.getBoundingBox());
	UTIL_ASSERT_REL(resolution, ==, materialized.getResolution());
	UTIL_ASSERT_REL(offset, ==, materialized.getOffset());
	UTIL_ASSERT_REL(dbb, ==, materialized.getDiscreteBoundingBox());

	return m;
}

void
UnionVolume::updateResolutionOffset() {

	// get union stats
	util::box<float, 3>   bb;
	util::point<float, 3> resolution;

	for (auto volume : _union) {

		bb += volume->getBoundingBox();

		if (resolution.isZero())
			resolution = volume->getResolution();
		else
			UTIL_ASSERT_REL(resolution, ==, volume->getResolution());
	}

	util::point<unsigned int, 3> discreteSize(
			bb.width() /resolution.x(),
			bb.height()/resolution.y(),
			bb.depth() /resolution.z());
	_discreteBb = util::box<unsigned int, 3>(
			util::point<unsigned int, 3>(),
			discreteSize);

	setResolution(resolution);
	setOffset(bb.min());
}
