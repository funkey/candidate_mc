#include "OverlapCosts.h"

OverlapCosts::OverlapCosts(
		const Crag&                crag,
		const ExplicitVolume<int>& groundTruth) :
	Costs(crag) {

	// for each root node
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		int numParents = 0;
		for (Crag::SubsetOutArcIt a(crag.getSubsetGraph(), crag.toSubset(n)); a != lemon::INVALID; ++a)
			numParents++;

		if (numParents == 0)
			recurseOverlapCosts(crag, n, groundTruth);
	}
}

std::set<Crag::Node>
OverlapCosts::recurseOverlapCosts(
		const Crag&                crag,
		const Crag::Node&          n,
		const ExplicitVolume<int>& groundTruth) {

	// get all leaf subnodes
	std::set<Crag::Node> subnodes;
	for (Crag::SubsetInArcIt a(crag.getSubsetGraph(), crag.toSubset(n)); a != lemon::INVALID; ++a) {

		std::set<Crag::Node> a_subnodes = recurseOverlapCosts(crag, crag.toRag(crag.getSubsetGraph().source(a)), groundTruth);

		for (Crag::Node s : a_subnodes)
			subnodes.insert(s);
	}

	// leaf node?
	if (subnodes.size() == 0) {

		node[n] = -overlap(crag.getVolumes()[n], groundTruth);
		subnodes.insert(n);

	} else {

		node[n] = 0;
		for (Crag::Node s : subnodes)
			node[n] += node[s];
	}

	return subnodes;
}

double
OverlapCosts::overlap(
		const ExplicitVolume<bool>& region,
		const ExplicitVolume<int>&  groundTruth) {

	std::map<int, int> overlaps;
	int maxOverlap = 0;

	util::point<unsigned int, 3> offset =
			(region.getOffset() - groundTruth.getOffset())/
			groundTruth.getResolution();

	for (unsigned int z = 0; z < region.getDiscreteBoundingBox().depth();  z++)
	for (unsigned int y = 0; y < region.getDiscreteBoundingBox().height(); y++)
	for (unsigned int x = 0; x < region.getDiscreteBoundingBox().width();  x++) {

		if (!region.data()(x, y, z))
			continue;

		int gtLabel = groundTruth[offset + util::point<unsigned int, 3>(x, y, z)];

		overlaps[gtLabel]++;
		maxOverlap = std::max(maxOverlap, overlaps[gtLabel]);
	}

	return maxOverlap;
}
