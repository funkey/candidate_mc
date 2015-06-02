#include "OverlapCosts.h"
#include <util/Logger.h>

logger::LogChannel overlapcostslog("overlapcostslog", "[OverlapCosts] ");

OverlapCosts::OverlapCosts(
		const Crag&                crag,
		const ExplicitVolume<int>& groundTruth) :
	Costs(crag),
	_overlaps(crag) {

	LOG_DEBUG(overlapcostslog) << "getting candidate overlaps..." << std::endl;

	// annotate all nodes with the max overlap area to any gt label
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		if (Crag::SubsetOutArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID)
			recurseOverlapCosts(crag, n, groundTruth);

	LOG_DEBUG(overlapcostslog) << "setting foreground overlap costs" << std::endl;

	// annotate nodes: costs is number of incorrectly merged pairs, minus number 
	// of correctly merged pairs
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		bool leafNode = (Crag::SubsetInArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID);

		//if (!leafNode) {

			//// scores only for leaf nodes
			//node[n] = 0;
			//continue;
		//}

		LOG_ALL(overlapcostslog)
				<< "getting foreground overlap score for node " << crag.id(n) << std::endl;

		node[n] = foregroundNodeOverlapScore(_overlaps[n]) + backgroundNodeOverlapScore(_overlaps[n]);

		LOG_ALL(overlapcostslog)
				<< "node " << crag.id(n)
				<< ": "    << node[n]
				<< std::endl;
	}

	// annotate edges: set score of combined overlaps
	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e) {

		Crag::Node u = crag.u(e);
		Crag::Node v = crag.v(e);

		bool leafEdge =
				(Crag::SubsetInArcIt(crag.getSubsetGraph(), crag.toSubset(u)) == lemon::INVALID &&
				 Crag::SubsetInArcIt(crag.getSubsetGraph(), crag.toSubset(v)) == lemon::INVALID);

		//if (!leafEdge) {

			//// scores only for leaf edges
			//edge[e] = 0;
			//continue;
		//}

		LOG_ALL(overlapcostslog)
				<< "getting foreground overlap score for "
				<< "edge (" << crag.id(crag.u(e))
				<< ", "     << crag.id(crag.v(e)) << ")"
				<< std::endl;

		edge[e] = foregroundEdgeOverlapScore(_overlaps[u], _overlaps[v]);

		LOG_ALL(overlapcostslog)
				<< "edge (" << crag.id(crag.u(e))
				<< ", "     << crag.id(crag.v(e))
				<< "): "    << edge[e] << std::endl;
	}
}

void
OverlapCosts::recurseOverlapCosts(
		const Crag&                crag,
		const Crag::Node&          n,
		const ExplicitVolume<int>& groundTruth) {

	bool leafNode = (Crag::SubsetInArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID);
	if (leafNode) {

		LOG_ALL(overlapcostslog) << "getting leaf overlap for node " << crag.id(n) << std::endl;
		_overlaps[n] = leafOverlaps(crag.getVolumes()[n], groundTruth);

	} else {

		// get overlaps with all gt regions
		for (Crag::SubsetInArcIt a(crag.getSubsetGraph(), crag.toSubset(n)); a != lemon::INVALID; ++a) {

			Crag::Node child = crag.toRag(crag.getSubsetGraph().source(a));

			recurseOverlapCosts(crag, child, groundTruth);

			for (auto p : _overlaps[child])
				if (!_overlaps[n].count(p.first))
					_overlaps[n][p.first] = p.second;
				else
					_overlaps[n][p.first] += p.second;
		}
	}
}

std::map<int, int>
OverlapCosts::leafOverlaps(
		const ExplicitVolume<bool>& region,
		const ExplicitVolume<int>&  groundTruth) {

	std::map<int, int> overlaps;

	util::point<unsigned int, 3> offset =
			(region.getOffset() - groundTruth.getOffset())/
			groundTruth.getResolution();

	LOG_ALL(overlapcostslog) << "offset into ground-truth image: " << offset << std::endl;

	for (unsigned int z = 0; z < region.getDiscreteBoundingBox().depth();  z++)
	for (unsigned int y = 0; y < region.getDiscreteBoundingBox().height(); y++)
	for (unsigned int x = 0; x < region.getDiscreteBoundingBox().width();  x++) {

		if (!region.data()(x, y, z))
			continue;

		int gtLabel = groundTruth[offset + util::point<unsigned int, 3>(x, y, z)];

		if (!overlaps.count(gtLabel))
			overlaps[gtLabel] = 1;
		else
			overlaps[gtLabel]++;
	}

	return overlaps;
}

double
OverlapCosts::foregroundNodeOverlapScore(
		const std::map<int, int>& overlaps) {

	double score = 0;

	// incorrectly merged pairs
	for (auto p : overlaps) {
		for (auto l : overlaps) {

			int label1 = p.first;
			int label2 = l.first;

			if (label1 >= label2)
				continue;

			double overlap1 = p.second;
			double overlap2 = l.second;

			LOG_ALL(overlapcostslog)
					<< "incorrectly merges " << label1
					<< " (" << overlap1 << " voxels) and " << label2
					<< " (" << overlap2 << " voxels)" << std::endl;

			score += overlap1*overlap2;
		}
	}

	// correctly merged pairs
	for (auto p : overlaps) {

		int label = p.first;
		if (label == 0)
			continue;

		double overlap = p.second;
		score -= overlap*(overlap - 1)/2;

		LOG_ALL(overlapcostslog)
				<< "correctly merges " << label
				<< " (" << overlap << " voxels)" << std::endl;
	}

	return score;
}

double
OverlapCosts::foregroundEdgeOverlapScore(
		const std::map<int, int>& overlapsU,
		const std::map<int, int>& overlapsV) {

	double score = 0;

	for (auto p : overlapsU) {
		for (auto l : overlapsV) {

			int label1 = p.first;
			int label2 = l.first;

			double overlap1 = p.second;
			double overlap2 = l.second;

			// correctly merged
			if (label1 == label2) {

				score -= overlap1*overlap2;

				LOG_ALL(overlapcostslog)
						<< "correctly merges " << label1
						<< " (" << overlap1 << " voxels) and " << label2
						<< " (" << overlap2 << " voxels)" << std::endl;

			// incorrectly merged
			} else {

				score += overlap1*overlap2;

				LOG_ALL(overlapcostslog)
						<< "incorrectly merges " << label1
						<< " (" << overlap1 << " voxels) and " << label2
						<< " (" << overlap2 << " voxels)" << std::endl;
			}
		}
	}

	return score;
}

double
OverlapCosts::backgroundNodeOverlapScore(
		const std::map<int, int>& overlaps) {

	if (!overlaps.count(0))
		return 0;

	// overlap with background
	double overlap = overlaps.at(0);

	// reward is -overlap^2 of not selecting this node, i.e., punishment of 
	// overlap^2
	return pow(overlap, 2);
}
