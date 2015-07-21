#include <util/assert.h>
#include "CragVolumes.h"

void
CragVolumes::setLeafNodeVolume(Crag::Node leafNode, std::shared_ptr<CragVolume> volume) {

	if (!_crag.isLeafNode(leafNode))
		UTIL_THROW_EXCEPTION(
				UsageError,
				"volumes can only be set for leaf nodes");

	_volumes[leafNode] = volume;
	setBoundingBoxDirty();
}

util::box<float, 3>
CragVolumes::getBoundingBox(Crag::Node n) const {

	// for leaf nodes, we assume the volume is known already

	if (_crag.isLeafNode(n)) {

		if (!_volumes[n])
			UTIL_THROW_EXCEPTION(
					UsageError,
					"no volume set for leaf node " << _crag.id(n));

		return _volumes[n]->getBoundingBox();
	}

	// if we have an explicit volume for the node already, just use that

	if (_volumes[n])
		return _volumes[n]->getBoundingBox();

	// otherwise, combine the children node's bounding boxes

	util::box<float, 3> bb;

	for (Crag::SubsetInArcIt e(_crag, _crag.toSubset(n)); e != lemon::INVALID; ++e)
		bb += getBoundingBox(_crag.toRag(_crag.getSubsetGraph().oppositeNode(_crag.toSubset(n), e)));

	return bb;
}

std::shared_ptr<CragVolume>
CragVolumes::operator[](Crag::Node n) const {

	if (!_volumes[n]) {

		// if there was no volume set or already computed, do it now

		_volumes[n] = std::make_shared<CragVolume>();

		const util::box<float, 3>& nodeBoundingBox = getBoundingBox(n);
		recFill(nodeBoundingBox, *_volumes[n], n);

		UTIL_ASSERT_REL(nodeBoundingBox, ==, _volumes[n]->getBoundingBox());
	}

	return _volumes[n];
}

void
CragVolumes::recFill(
		const util::box<float, 3>& boundingBox,
		CragVolume&                volume,
		Crag::Node                 n) const {

	if (_crag.isLeafNode(n)) {

		// if n is a leaf node, just copy the voxels in the target volume

		const CragVolume& leaf = *_volumes[n];

		UTIL_ASSERT(!leaf.getBoundingBox().isZero());

		util::point<unsigned int, 3> volumeOffset =
				boundingBox.min()/
				leaf.getResolution();
		util::point<unsigned int, 3> leafOffset =
				leaf.getBoundingBox().min()/
				leaf.getResolution();
		util::point<unsigned int, 3> offset = leafOffset - volumeOffset;

		if (volume.getBoundingBox().isZero()) {

			volume = CragVolume(
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

	} else {

		// if n is a compound node, copy all children's volumes in the target 
		// volume

		for (Crag::SubsetInArcIt e(_crag, _crag.toSubset(n)); e != lemon::INVALID; ++e)
			recFill(boundingBox, volume, _crag.toRag(_crag.getSubsetGraph().oppositeNode(_crag.toSubset(n), e)));
	}
}
