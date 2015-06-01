#include "OverlapCosts.h"

OverlapCosts::OverlapCosts(
		const Crag&                crag,
		const ExplicitVolume<int>& groundTruth) :
	Costs(crag),
	_bestLabels(crag) {

	// annotate all nodes with the max overlap area to any gt label
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		if (Crag::SubsetOutArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID)
			recurseOverlapCosts(crag, n, groundTruth);

	// for each edge, set score proportional to product of max overlaps of the 
	// involved nodes, if the nodes overlap with the same label
	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e) {

		Crag::Node u = crag.u(e);
		Crag::Node v = crag.v(e);

		int overlapU = node[u];
		int overlapV = node[v];

		if (_bestLabels[u] == _bestLabels[v])
			edge[e] = -2*overlapU*overlapV;
		else
			edge[e] = 0;
	}

	// finalize overlap scores
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		node[n] = -pow(node[n], 2);
}

std::map<int, int>
OverlapCosts::recurseOverlapCosts(
		const Crag&                crag,
		const Crag::Node&          n,
		const ExplicitVolume<int>& groundTruth) {

	// get overlaps with all gt regions
	std::map<int, int> overlaps;
	for (Crag::SubsetInArcIt a(crag.getSubsetGraph(), crag.toSubset(n)); a != lemon::INVALID; ++a) {

		std::map<int, int> childOverlaps = recurseOverlapCosts(crag, crag.toRag(crag.getSubsetGraph().source(a)), groundTruth);

		for (auto p : childOverlaps)
			overlaps[p.first] += p.second;
	}

	// we are a leaf node
	if (overlaps.size() == 0)
		overlaps = leafOverlaps(crag.getVolumes()[n], groundTruth);

	int maxOverlap = 0;
	int bestLabel  = 0;
	for (auto p : overlaps)
		if (p.second >= maxOverlap) {

			maxOverlap = p.second;
			bestLabel  = p.first;
		}

	node[n]        = maxOverlap;
	_bestLabels[n] = bestLabel;

	return overlaps;
}

std::map<int, int>
OverlapCosts::leafOverlaps(
		const ExplicitVolume<bool>& region,
		const ExplicitVolume<int>&  groundTruth) {

	std::map<int, int> overlaps;

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
	}

	return overlaps;
}
