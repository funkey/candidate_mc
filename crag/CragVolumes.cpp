#include "CragVolumes.h"
#include <util/Logger.h>
#include <util/assert.h>

logger::LogChannel cragvolumeslog("cragvolumeslog", "[CragVolumes] ");

CragVolumes::CragVolumes(const Crag& crag) :
	_crag(crag),
	_volumes(crag) {

	_cache.set_max_size(1024);
}

void
CragVolumes::setVolume(Crag::CragNode n, std::shared_ptr<CragVolume> volume) {

	_volumes[n] = UnionVolume(volume);
	setBoundingBoxDirty();
}

std::shared_ptr<CragVolume>
CragVolumes::operator[](Crag::CragNode n) const {

	// if this is already a leaf node volume, no need to materialize
	if (_volumes[n].numUnionVolumes() == 1)
		return _volumes[n].getUnionVolume(0);

	// if this volume is empty, we need to find all leaf node volumes that 
	// create it
	if (_volumes[n].numUnionVolumes() == 0) {

		if (_crag.isLeafNode(n))
			UTIL_THROW_EXCEPTION(
					UsageError,
					"node " << _crag.id(n) << " is a leaf node but has no volume assigned");

		auto leafNodes = _crag.leafNodes(n);
		std::vector<std::shared_ptr<CragVolume>> leafVolumes;

		for (Crag::CragNode l : leafNodes)
			leafVolumes.push_back(this->operator[](l));
		_volumes[n] = UnionVolume(leafVolumes);
	}

	UnionVolume& v = _volumes[n];
	return _cache.get(n, [v]{ return v.materialize(); });
}

bool
CragVolumes::is2D() const {

	for (Crag::CragNode n : _crag.nodes())
		if (_crag.type(n) != Crag::SliceNode)
			return false;

	return true;
}

void
CragVolumes::clearCache() {

	_cache.clear();
}
