#include "HausdorffDistance.h"
#include <vigra/functorexpression.hxx>
#include <vigra/distancetransform.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/impex.hxx>
#include <util/timing.h>

HausdorffDistance::HausdorffDistance(
		const CragVolumes& a,
		const CragVolumes& b,
		int maxDistance) :
	_a(a),
	_b(b),
	_maxDistance(maxDistance) {

	_leafNodesA = collectLeafNodes(a.getCrag());
	_leafNodesB = collectLeafNodes(b.getCrag());
}

void
HausdorffDistance::distance(Crag::Node i, Crag::Node j, double& i_j, double& j_i) {

	//std::cout << "getting distance between " << _a.id(i) << " and " << _b.id(j) << std::endl;

	double maxDistance_i_j = 0;
	double maxDistance_j_i = 0;

	for (Crag::Node iLeaf : _leafNodesA[i]) {

		double minDistance_i_j = std::numeric_limits<double>::infinity();
		double minDistance_j_i = std::numeric_limits<double>::infinity();

		for (Crag::Node jLeaf : _leafNodesA[j]) {

			double distance_i_j, distance_j_i;
			leafDistance(iLeaf, jLeaf, distance_i_j, distance_j_i);

			minDistance_i_j = std::min(minDistance_i_j, distance_i_j);
			minDistance_j_i = std::min(minDistance_j_i, distance_j_i);
		}

		maxDistance_i_j = std::max(maxDistance_i_j, minDistance_i_j);
		maxDistance_j_i = std::max(maxDistance_j_i, minDistance_j_i);
	}

	i_j = maxDistance_i_j;
	j_i = maxDistance_j_i;
}

void
HausdorffDistance::leafDistance(Crag::Node i, Crag::Node j, double& i_j, double& j_i) {

	if (_cache.count(std::make_pair(i, j))) {

		i_j = _cache[std::make_pair(i, j)].first;
		j_i = _cache[std::make_pair(i, j)].second;

		return;
	}

	//std::cout << "checking leaf nodes " << _a.id(i) << "a and " << _b.id(j) << "b" << std::endl;
	leafDistance(i, j, i_j);

	//std::cout << "checking leaf nodes " << _b.id(j) << "b and " << _a.id(i) << "a" << std::endl;
	leafDistance(j, i, j_i);

	_cache[std::make_pair(i, j)] = std::make_pair(i_j, j_i);
}

void
HausdorffDistance::leafDistance(
		Crag::Node i,
		Crag::Node j,
		double& i_j) {

	const CragVolume& volume_i = *_a[i];
	const CragVolume& volume_j = *_a[j];

	const util::box<int, 2>& bb_i = (volume_i.getBoundingBox()/volume_i.getResolution()).project<2>();
	const util::box<int, 2>& bb_j = (volume_j.getBoundingBox()/volume_j.getResolution()).project<2>();

	//std::cout << "bb_i: " << bb_i << " " << volume_i.getBoundingBox() << std::endl;
	//std::cout << "bb_j: " << bb_j << " " << volume_j.getBoundingBox() << std::endl;

	vigra::MultiArray<2, double>& distances_j = getDistanceMap(volume_j);

	//vigra::exportImage(distances_j, vigra::ImageExportInfo((std::string("distance_") + boost::lexical_cast<std::string>(_b.id(j)) + ".png").c_str()));

	double maxDistance = 0;
	for (int y = 0; y < bb_i.height(); y++)
	for (int x = 0; x < bb_i.width();  x++) {

		if (!volume_i(x, y, 0))
			continue;

		// point in global coordinates
		util::point<int, 2> p = bb_i.min() + util::point<int, 2>(x, y);

		// point relative to bb_j.min()
		util::point<int, 2> p_j = p - bb_j.min();

		// point relative to distance map
		util::point<int, 2> p_d = p_j + util::point<int, 2>(_maxDistance, _maxDistance);

		double distance;
		if (p_d.x() >= distances_j.shape(0) || p_d.y() >= distances_j.shape(1) || p_d.x() < 0 || p_d.y() < 0) {

			distance = _maxDistance;

			//std::cout << "point " << p << " not within " << _maxDistance << " to j" << std::endl;

		} else {

			distance = distances_j(p_d.x(), p_d.y());

			//std::cout << "point " << p << "(" << p_d << "in distance map) has distance " << distance << " to j" << std::endl;
		}

		maxDistance = std::max(maxDistance, distance);
	}

	i_j = maxDistance;

	//std::cout << "distance: " << i_j << std::endl;
}

vigra::MultiArray<2, double>&
HausdorffDistance::getDistanceMap(const ExplicitVolume<unsigned char>& volume) {

	UTIL_TIME_METHOD;

	if (_distanceMaps.count(&volume))
		return _distanceMaps[&volume];

	vigra::Shape2 size(volume.width() + 2*_maxDistance, volume.height() + 2*_maxDistance);

	vigra::MultiArray<2, double>& distanceMap = _distanceMaps[&volume];
	distanceMap.reshape(size);
	distanceMap = 0;

	vigra::copyMultiArray(
			volume.data().bind<2>(0),
			distanceMap.subarray(
					vigra::Shape2(
							_maxDistance,
							_maxDistance),
					vigra::Shape2(
							_maxDistance + volume.width(),
							_maxDistance + volume.height())));

	// perform distance transform with Euclidean norm
	vigra::distanceTransform(srcImageRange(distanceMap), destImage(distanceMap), 0.0, 2);

	using namespace vigra::functor;

	// cut values to maxDistance
	vigra::transformImage(srcImageRange(distanceMap), destImage(distanceMap), min(Arg1(), Param(_maxDistance)));

	return distanceMap;
}

HausdorffDistance::LeafNodeMap
HausdorffDistance::collectLeafNodes(const Crag& crag) {

	LeafNodeMap map;

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		if (Crag::SubsetOutArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID)
			recCollectLeafNodes(crag, n, map);

	return map;
}

void
HausdorffDistance::recCollectLeafNodes(const Crag& crag, Crag::Node n, LeafNodeMap& map) {

	int numChildren = 0;
	for (Crag::SubsetInArcIt c(crag.getSubsetGraph(), crag.toSubset(n)); c != lemon::INVALID; ++c) {

		Crag::Node child = crag.toRag(crag.getSubsetGraph().oppositeNode(crag.toSubset(n), c));

		recCollectLeafNodes(crag, child, map);
		std::copy(map[child].begin(), map[child].end(), std::back_inserter(map[n]));

		numChildren++;
	}

	if (numChildren == 0)
		map[n].push_back(n);
}
