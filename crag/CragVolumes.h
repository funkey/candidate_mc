#ifndef CANDIDATE_MC_CRAG_CRAG_VOLUMES_H__
#define CANDIDATE_MC_CRAG_CRAG_VOLUMES_H__

#include <memory>
#include <imageprocessing/ExplicitVolume.h>
#include <util/cache.hpp>
#include "Crag.h"
#include "CragVolume.h"
#include "UnionVolume.h"

/**
 * A node property map for Crags that provides the volumes of candidates as 
 * CragVolume.
 */
class CragVolumes : public Volume {

public:

	/**
	 * Create an empty volume map for the given Crag. Populate it using 
	 * setVolume() for each leaf node.
	 */
	CragVolumes(const Crag& crag);

	virtual ~CragVolumes() {}

	CragVolumes(CragVolumes&& other) :
		_crag(other._crag),
		_volumes(other._crag) {

		for (Crag::CragNode n : _crag.nodes()) {

			_volumes[n] = other._volumes[n];
			other._volumes[n].clear();
		}
	}

	/**
	 * Set the volume of a leaf node.
	 */
	void setVolume(Crag::CragNode n, std::shared_ptr<CragVolume> volume);

	/**
	 * Get the volume of a candidate. If the candidate is a higher candidate, 
	 * it's volume will be materialized from the leaf node volume it merges.
	 */
	std::shared_ptr<CragVolume> operator[](Crag::CragNode n) const;

	/**
	 * Get the bounding box of all volumes combined.
	 */
	using Volume::getBoundingBox;

	/**
	 * Get the bounding box of a volume.
	 *
	 * This does not materialize the volume and should be preferred over 
	 * volumes[n].getBoundingBox().
	 */
	util::box<float,3> getBoundingBox(Crag::CragNode n) const {
		return _volumes[n].getBoundingBox();
	}

	/**
	 * Get the Crag associated to the volumes.
	 */
	const Crag& getCrag() const { return _crag; }

	/**
	 * Return true if all the volumes are 2D slices.
	 */
	bool is2D() const;

	/**
	 * Clear volumes of higher-order nodes that have been generated on-the-fly 
	 * by operator[]().
	 */
	void clearCache();

protected:

	util::box<float,3> computeBoundingBox() const override {

		util::box<float, 3> bb;
		for (Crag::CragNode n : _crag.nodes())
			if (!_volumes[n].getBoundingBox().isZero())
				bb += operator[](n)->getBoundingBox();

		return bb;
	}

private:

	const Crag& _crag;

	mutable Crag::NodeMap<UnionVolume> _volumes;
	mutable cache<Crag::CragNode, std::shared_ptr<CragVolume>> _cache;
};

#endif // CANDIDATE_MC_CRAG_CRAG_VOLUMES_H__

