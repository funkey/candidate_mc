#ifndef CANDIDATE_MC_UNION_VOLUME_H__
#define CANDIDATE_MC_UNION_VOLUME_H__

#include <vector>
#include <imageprocessing/DiscreteVolume.h>
#include <util/assert.h>
#include "CragVolume.h"

/**
 * Union of several CragVolumes.
 */
class UnionVolume : public DiscreteVolume {

public:

	UnionVolume() {}

	/**
	 * Create a UnionVolume from a single CragVolume.
	 */
	UnionVolume(std::shared_ptr<CragVolume> volume) {

		_union.push_back(volume);
	}

	/**
	 * Create a UnionVolume from a vector of CragVolume.
	 */
	UnionVolume(std::vector<std::shared_ptr<CragVolume>> volumes) {

		_union = volumes;
	}

	bool clear() {

		if (numUnionVolumes() == 0)
			return false;

		_union.clear();
		setDiscreteBoundingBoxDirty();

		return true;
	}

	size_t numUnionVolumes() const { return _union.size(); }

	std::shared_ptr<CragVolume> getUnionVolume(size_t i) const { return _union[i]; }

	/**
	 * Convert this UnionVolume into a CragVolume.
	 */
	std::shared_ptr<CragVolume> materialize() const {

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

		// create a new volume
		auto materialized = std::make_shared<CragVolume>(
				bb.width() /resolution.x(),
				bb.height()/resolution.y(),
				bb.depth() /resolution.z());
		materialized->setOffset(bb.min());
		materialized->setResolution(resolution);

		// descrete offset of materialized in global volume
		util::point<unsigned int, 3> materializedOffset = bb.min()/resolution;

		// combine union
		for (auto volume : _union) {

			// descrete offset of volume in global volume
			util::point<unsigned int, 3> volumeOffset =
					volume->getBoundingBox().min()/
					volume->getResolution();

			// offset to get from positions in materialized to positions in 
			// volume
			util::point<unsigned int, 3> offset = volumeOffset - materializedOffset;

			// copy volume into materialized
			for (unsigned int z = 0; z < volume->depth();  z++)
			for (unsigned int y = 0; y < volume->height(); y++)
			for (unsigned int x = 0; x < volume->width();  x++) {

				if ((*volume)(x, y, z) > 0)
					(*materialized)(
							offset.x() + x,
							offset.y() + y,
							offset.z() + z) = (*volume)(x, y, z);
			}
		}

		UTIL_ASSERT_REL(bb, ==, materialized->getBoundingBox());
		return materialized;
	}

protected:

	util::box<unsigned int,3> computeDiscreteBoundingBox() const override {

		util::box<unsigned int, 3> bb;
		for (auto v : _union)
			bb += v->getDiscreteBoundingBox();
		return bb;
	}

private:

	std::vector<std::shared_ptr<CragVolume>> _union;
};

#endif // CANDIDATE_MC_UNION_VOLUME_H__

