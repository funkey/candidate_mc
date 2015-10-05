#include "CragVolumes.h"
#include <util/Logger.h>
#include <util/assert.h>

logger::LogChannel cragvolumeslog("cragvolumeslog", "[CragVolumes] ");

void
CragVolumes::setVolume(Crag::CragNode n, std::shared_ptr<CragVolume> volume) {

	_volumes[n] = volume;
	setBoundingBoxDirty();
}

void
CragVolumes::fillEmptyVolumes() {

	// fill empty volumes by recursively combining children, starting from all 
	// root nodes
	for (Crag::CragNode n : _crag.nodes()) {

		if (!_crag.isRootNode(n) || _crag.isLeafNode(n))
			continue;

		recFill(n);
	}
}

std::shared_ptr<CragVolume>
CragVolumes::operator[](Crag::CragNode n) const {

	if (!_volumes[n])
		UTIL_THROW_EXCEPTION(
				UsageError,
				"no volume set for node " << _crag.id(n));

	return _volumes[n];
}

void
CragVolumes::recFill(Crag::CragNode n) {

	LOG_ALL(cragvolumeslog) << "filling node " << _crag.id(n) << std::endl;

	// if n already has a volume, there is nothing to do
	if (_volumes[n] && !_volumes[n]->getBoundingBox().isZero()) {

		LOG_ALL(cragvolumeslog) << "\talready has a volume" << std::endl;
		return;
	}

	// fill all children
	util::box<float, 3> bb;
	util::point<float, 3> childrenResolution;
	for (Crag::CragArc childArc : _crag.inArcs(n)) {

		Crag::CragNode child = childArc.source();

		LOG_ALL(cragvolumeslog) << "\tprocessing child" << std::endl;
		recFill(child);
		LOG_ALL(cragvolumeslog) << "\tdone processing child" << std::endl;

		bb += _volumes[child]->getBoundingBox();

		LOG_ALL(cragvolumeslog) << "\tchild bounding box is " << _volumes[child]->getBoundingBox() << std::endl;

		if (childrenResolution.isZero())
			childrenResolution = _volumes[child]->getResolution();
		else
			UTIL_ASSERT_REL(childrenResolution, ==, _volumes[child]->getResolution());

		LOG_ALL(cragvolumeslog) << "\taccumulated bounding box is " << bb << std::endl;
	}

	// create a new volume for this node
	_volumes[n] = std::make_shared<CragVolume>(
			bb.width() /childrenResolution.x(),
			bb.height()/childrenResolution.y(),
			bb.depth() /childrenResolution.z());
	CragVolume& nVolume = *_volumes[n];
	nVolume.setOffset(bb.min());
	nVolume.setResolution(childrenResolution);

	// descrete offset of n in global volume
	util::point<unsigned int, 3> volumeOffset = bb.min()/childrenResolution;

	// combine children
	for (Crag::CragArc childArc : _crag.inArcs(n)) {

		Crag::CragNode child = childArc.source();
		const CragVolume& childVolume = *_volumes[child];

		// descrete offset of child in global volume
		util::point<unsigned int, 3> childOffset =
				childVolume.getBoundingBox().min()/
				childVolume.getResolution();

		// offset to get from positions in n to positions in child
		util::point<unsigned int, 3> offset = childOffset - volumeOffset;

		// copy childVolume into nVolume
		for (unsigned int z = 0; z < childVolume.depth();  z++)
		for (unsigned int y = 0; y < childVolume.height(); y++)
		for (unsigned int x = 0; x < childVolume.width();  x++) {

			if (childVolume(x, y, z) > 0)
				nVolume(
						offset.x() + x,
						offset.y() + y,
						offset.z() + z) = childVolume(x, y, z);
		}
	}

	UTIL_ASSERT_REL(bb, ==, _volumes[n]->getBoundingBox());
}
