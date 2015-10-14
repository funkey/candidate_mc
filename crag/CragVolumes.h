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

		for (Crag::CragNode n : _crag.nodes()) {

			_volumes[n] = other._volumes[n];
			other._volumes[n].reset();
		}
	}

	/**
	 * Set the volume of a node.
	 */
	void setVolume(Crag::CragNode n, std::shared_ptr<CragVolume> volume);

	/**
	 * Fill empty volumes with the unions of their child volumes. If the child 
	 * volumes are empty, too, this function is invoked recursively. Assumes 
	 * that all leaf nodes have a volume and that empty volumes have only empty 
	 * parents.
	 */
	void fillEmptyVolumes();

	/**
	 * Get the volume of a candidate. Don't use this operator to set volumes, 
	 * use setVolume() instead.
	 */
	std::shared_ptr<CragVolume> operator[](Crag::CragNode n) const;

	/**
	 * Get the bounding box of all volumes combined.
	 */
	using Volume::getBoundingBox;

	/**
	 * Get the Crag associated to the volumes.
	 */
	const Crag& getCrag() const { return _crag; }

	/**
	 * Return true if all the volumes are 2D slices.
	 */
	bool is2D() const;

protected:

	util::box<float,3> computeBoundingBox() const override {

		util::box<float, 3> bb;
		for (Crag::CragNode n : _crag.nodes())
			if (_volumes[n])
				bb += operator[](n)->getBoundingBox();

		return bb;
	}

private:

	// Fill the volume of each empty node under (including) n with the union of 
	// the nodes ancestors.
	void recFill(Crag::CragNode n);

	const Crag& _crag;

	Crag::NodeMap<std::shared_ptr<CragVolume>> _volumes;
};

#endif // CANDIDATE_MC_CRAG_CRAG_VOLUMES_H__

