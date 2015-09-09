#ifndef CANDIDATE_MC_CRAG_CRAG_VOLUMES_H__
#define CANDIDATE_MC_CRAG_CRAG_VOLUMES_H__

#include <memory>
#include <imageprocessing/ExplicitVolume.h>
#include "Crag.h"
#include "CragVolume.h"

/**
 * A node property map for Crags that provides the volumes of candidates as 
 * CragVolume.
 */
class CragVolumes : public Volume {

public:

	/**
	 * Create an empty volume map for the given Crag. Populate it using 
	 * setVolume() for each node or setVolume() for each leaf node and 
	 * propagateLeafNodeVolumes().
	 */
	CragVolumes(const Crag& crag) :
		_crag(crag),
		_volumes(crag) {}

	virtual ~CragVolumes() {}

	CragVolumes(CragVolumes&& other) :
		_crag(other._crag),
		_volumes(other._crag) {

		for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

			_volumes[n] = other._volumes[n];
			other._volumes[n].reset();
		}
	}

	/**
	 * Set the volume of a node.
	 */
	void setVolume(Crag::Node n, std::shared_ptr<CragVolume> volume);

	/**
	 * Propagate the volumes of leaf nodes upwards, such that each higher volume 
	 * is simply the union of its candidates.
	 */
	void propagateLeafNodeVolumes();

	/**
	 * Get the volume of a candidate.
	 */
	std::shared_ptr<CragVolume> operator[](Crag::Node n) const;

	/**
	 * Get the bounding box of a candidate.
	 */
	util::box<float, 3> getBoundingBox(Crag::Node n) const;

	/**
	 * Get the bounding box of all volumes combined.
	 */
	using Volume::getBoundingBox;

	/**
	 * Get the Crag associated to the volumes.
	 */
	const Crag& getCrag() const { return _crag; }

protected:

	util::box<float,3> computeBoundingBox() const override {

		util::box<float, 3> bb;
		for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
			if (_volumes[n])
				bb += operator[](n)->getBoundingBox();

		return bb;
	}

private:

	void recFill(
			const util::box<float, 3>& boundingBox,
			CragVolume&                volume,
			Crag::Node                 n) const;

	const Crag& _crag;

	mutable Crag::NodeMap<std::shared_ptr<CragVolume>> _volumes;
};

#endif // CANDIDATE_MC_CRAG_CRAG_VOLUMES_H__

