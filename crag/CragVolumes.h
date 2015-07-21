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
	 * Create a volume map for the given Crag. The volumes of leaf nodes are 
	 * read from the given leaf node labels volume, where each voxel value is 
	 * the id of the leaf node it belongs to.
	 */
	CragVolumes(
			const Crag& crag,
			std::shared_ptr<ExplicitVolume<unsigned int>> leafNodeLabels) :
		_crag(crag),
		_volumes(crag),
		_leafNodeLabels(leafNodeLabels) {}

	/**
	 * Create an empty volume map for the given Crag. Leaf node volumes have to 
	 * be set explicitly via setLeafNodeVolume() before the map can be used.
	 */
	CragVolumes(const Crag& crag) :
		_crag(crag),
		_volumes(crag) {}

	CragVolumes(CragVolumes&& other) :
		_crag(other._crag),
		_volumes(other._crag),
		_leafNodeLabels(other._leafNodeLabels) {

		for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

			_volumes[n] = other._volumes[n];
			other._volumes[n].reset();
		}

		other._leafNodeLabels.reset();
	}

	/**
	 * Set the volume of a leaf node.
	 */
	void setLeafNodeVolume(Crag::Node leafNode, std::shared_ptr<CragVolume> volume);

	/**
	 * Get the volume of a candidate. For non-leaf node candidates, the volume 
	 * will be created and cached for later queries.
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
			if (_crag.isLeafNode(n))
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

	std::shared_ptr<ExplicitVolume<unsigned int>> _leafNodeLabels;
};

#endif // CANDIDATE_MC_CRAG_CRAG_VOLUMES_H__

