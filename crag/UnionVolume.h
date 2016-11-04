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
		updateResolutionOffset();
		setDiscreteBoundingBoxDirty();
	}

	/**
	 * Create a UnionVolume from a vector of CragVolume.
	 */
	UnionVolume(std::vector<std::shared_ptr<CragVolume>> volumes) {

		_union = volumes;
		updateResolutionOffset();
		setDiscreteBoundingBoxDirty();
	}

	bool clear() {

		if (numUnionVolumes() == 0)
			return false;

		_union.clear();

		setResolution(util::point<float,3>(1,1,1));
		setOffset(util::point<float,3>(0,0,0));
		setDiscreteBoundingBoxDirty();

		return true;
	}

	size_t numUnionVolumes() const { return _union.size(); }

	std::shared_ptr<CragVolume> getUnionVolume(size_t i) const { return _union[i]; }

	/**
	 * Convert this UnionVolume into a CragVolume.
	 */
	std::shared_ptr<CragVolume> materialize() const;

protected:

	util::box<unsigned int,3> computeDiscreteBoundingBox() const override {

		return _discreteBb;
	}

private:

	void updateResolutionOffset();

	std::vector<std::shared_ptr<CragVolume>> _union;
	util::box<unsigned int, 3> _discreteBb;
};

#endif // CANDIDATE_MC_UNION_VOLUME_H__

