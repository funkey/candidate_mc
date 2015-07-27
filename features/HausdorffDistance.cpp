#include "HausdorffDistance.h"
#include <vigra/functorexpression.hxx>
#include <vigra/multi_distance.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/impex.hxx>
#include <util/timing.h>
#include <util/Logger.h>

logger::LogChannel hausdorffdistancelog("hausdorffdistancelog", "[HausdorffDistance] ");

HausdorffDistance::HausdorffDistance(
		const CragVolumes& a,
		const CragVolumes& b,
		int maxDistance) :
	_a(a),
	_b(b),
	_maxDistance(maxDistance) {}

void
HausdorffDistance::operator()(Crag::Node i, Crag::Node j, double& i_j, double& j_i) {

	if (_cache.count(std::make_pair(i, j))) {

		LOG_DEBUG(hausdorffdistancelog) << "reuse cached values" << std::endl;

		i_j = _cache[std::make_pair(i, j)].first;
		j_i = _cache[std::make_pair(i, j)].second;

		return;
	}

	LOG_DEBUG(hausdorffdistancelog)
			<< "checking nodes " << _a.getCrag().id(i)
			<< "a and " << _b.getCrag().id(j) << "b"
			<< std::endl;

	volumesDistance(_a[i], _b[j], i_j);

	LOG_DEBUG(hausdorffdistancelog)
			<< "checking nodes " << _b.getCrag().id(j)
			<< "b and " << _a.getCrag().id(i) << "a"
			<< std::endl;

	volumesDistance(_b[j], _a[i], j_i);

	_cache[std::make_pair(i, j)] = std::make_pair(i_j, j_i);
}

void
HausdorffDistance::volumesDistance(
		std::shared_ptr<CragVolume> volume_i,
		std::shared_ptr<CragVolume> volume_j,
		double& i_j) {

	if (lowerBound(*volume_i, *volume_j) >= _maxDistance) {

		i_j = _maxDistance;
		return;
	}

	const util::box<int, 2>& bb_i = (volume_i->getBoundingBox()/volume_i->getResolution()).project<2>();
	const util::box<int, 2>& bb_j = (volume_j->getBoundingBox()/volume_j->getResolution()).project<2>();

	LOG_DEBUG(hausdorffdistancelog) << "bb_i: " << bb_i << " " << volume_i->getBoundingBox() << std::endl;
	LOG_DEBUG(hausdorffdistancelog) << "bb_j: " << bb_j << " " << volume_j->getBoundingBox() << std::endl;

	vigra::MultiArray<2, double>& distances_j = getDistanceMap(volume_j);

	double maxDistance = 0;
	for (int y = 0; y < bb_i.height(); y++)
	for (int x = 0; x < bb_i.width();  x++) {

		if (!(*volume_i)(x, y, 0))
			continue;

		// point in global coordinates
		util::point<int, 2> p = bb_i.min() + util::point<int, 2>(x, y);

		// point relative to bb_j.min()
		util::point<int, 2> p_j = p - bb_j.min();

		LOG_ALL(hausdorffdistancelog) << "point " << p << " in i corresponds to point " << p_j << " in j" << std::endl;

		// point relative to distance map
		util::point<int, 2> p_d = p_j + util::point<int, 2>(_padX, _padY);

		LOG_ALL(hausdorffdistancelog) << "point " << p << " in i corresponds to point " << p_d << " in distance map of j" << std::endl;

		double distance;
		// not in distance map?
		if (p_d.x() >= distances_j.shape(0) || p_d.y() >= distances_j.shape(1) || p_d.x() < 0 || p_d.y() < 0) {

			distance = _maxDistance;

			LOG_ALL(hausdorffdistancelog) << "point " << p << " not within " << _maxDistance << " to j" << std::endl;

		} else {

			distance = std::min(_maxDistance, sqrt(distances_j(p_d.x(), p_d.y())));

			LOG_ALL(hausdorffdistancelog) << "point " << p << " has distance " << distance << " to j" << std::endl;
		}

		maxDistance = std::max(maxDistance, distance);
	}

	i_j = maxDistance;

	LOG_DEBUG(hausdorffdistancelog) << "distance: " << i_j << std::endl;
}

double
HausdorffDistance::lowerBound(const CragVolume& a, const CragVolume& b) {

	// get max x separation
	double maxSeparationX =
			std::max(
					b.getBoundingBox().min().x() - a.getBoundingBox().min().x(),
					a.getBoundingBox().max().x() - b.getBoundingBox().max().x());

	// get max y separation
	double maxSeparationY =
			std::max(
					b.getBoundingBox().min().y() - a.getBoundingBox().min().y(),
					a.getBoundingBox().max().y() - b.getBoundingBox().max().y());

	return std::max(maxSeparationX, maxSeparationY);
}

vigra::MultiArray<2, double>&
HausdorffDistance::getDistanceMap(std::shared_ptr<CragVolume> volume) {

	UTIL_TIME_METHOD;

	if (_distanceMaps.count(volume))
		return _distanceMaps[volume];

	_padX = (int)(ceil(_maxDistance/volume->getResolutionX()));
	_padY = (int)(ceil(_maxDistance/volume->getResolutionY()));

	vigra::Shape2 size(volume->width() + 2*_padX, volume->height() + 2*_padY);

	vigra::MultiArray<2, double>& distanceMap = _distanceMaps[volume];
	distanceMap.reshape(size);
	distanceMap = 0;

	vigra::copyMultiArray(
			volume->data().bind<2>(0),
			distanceMap.subarray(
					vigra::Shape2(
							_padX,
							_padY),
					vigra::Shape2(
							_padX + volume->width(),
							_padY + volume->height())));

	double pitch[2];
	pitch[0] = volume->getResolutionX();
	pitch[1] = volume->getResolutionY();


	// perform distance transform with Euclidean norm
	vigra::separableMultiDistSquared(
			distanceMap,
			distanceMap,
			true, /* get distance from object */
			pitch);

	return distanceMap;
}

