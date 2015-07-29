#include <limits>
#include "OverlapLoss.h"
#include <util/Logger.h>
#include <util/ProgramOptions.h>

util::ProgramOption optionBalanceOverlapLoss(
		util::_module           = "loss.overlap",
		util::_long_name        = "balance",
		util::_description_text = "Compute the overlap loss only for leaf node and edges, "
		                          "and propagate the values upwards, such that each solution "
		                          "resulting in the same segmentation has the same loss. Note "
		                          "that this sacrifices approximation quality of the overlap "
		                          "loss, since even fewer transitive contributions are considered.");

util::ProgramOption optionRestrictOverlapLossToLeaves(
		util::_module           = "loss.overlap",
		util::_long_name        = "restrictToLeaves",
		util::_description_text = "Compute the overlap loss only for leaf node and edges, and "
		                          "set all other variable losses to a large value, such that "
		                          "they will not be picked.");

logger::LogChannel overlaplosslog("overlaplosslog", "[OverlapLoss] ");

OverlapLoss::OverlapLoss(
		const Crag&                crag,
		const CragVolumes&         volumes,
		const ExplicitVolume<int>& groundTruth) :
	Loss(crag),
	_overlaps(crag) {

	bool balance = optionBalanceOverlapLoss;
	bool restrictToLeaves = optionRestrictOverlapLossToLeaves;

	LOG_DEBUG(overlaplosslog) << "getting candidate overlaps..." << std::endl;

	// annotate all nodes with the max overlap area to any gt label
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		if (Crag::SubsetOutArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID)
			recurseOverlapLoss(crag, volumes, n, groundTruth);

	LOG_DEBUG(overlaplosslog) << "setting foreground overlap loss" << std::endl;

	// annotate nodes: loss is number of incorrectly merged pairs, minus number 
	// of correctly merged pairs
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		bool leafNode = (Crag::SubsetInArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID);

		if ((balance || restrictToLeaves) && !leafNode) {

			node[n] = (restrictToLeaves ? std::numeric_limits<double>::infinity() : 0);
			continue;
		}

		LOG_ALL(overlaplosslog)
				<< "getting overlap score for node " << crag.id(n) << std::endl;

		node[n] = foregroundNodeOverlapScore(_overlaps[n]) + backgroundNodeOverlapScore(_overlaps[n]);

		LOG_ALL(overlaplosslog)
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

		if ((balance || restrictToLeaves) && !leafEdge) {

			// scores only for leaf edges
			edge[e] = (restrictToLeaves ? std::numeric_limits<double>::infinity() : 0);
			continue;
		}

		LOG_ALL(overlaplosslog)
				<< "getting foreground overlap score for "
				<< "edge (" << crag.id(crag.u(e))
				<< ", "     << crag.id(crag.v(e)) << ")"
				<< std::endl;

		edge[e] = foregroundEdgeOverlapScore(_overlaps[u], _overlaps[v]);

		LOG_ALL(overlaplosslog)
				<< "edge (" << crag.id(crag.u(e))
				<< ", "     << crag.id(crag.v(e))
				<< "): "    << edge[e] << std::endl;
	}

	if (balance)
		propagateLeafLoss(crag);
}

void
OverlapLoss::recurseOverlapLoss(
		const Crag&                crag,
		const CragVolumes&         volumes,
		const Crag::Node&          n,
		const ExplicitVolume<int>& groundTruth) {

	bool leafNode = (Crag::SubsetInArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID);
	if (leafNode) {

		LOG_ALL(overlaplosslog) << "getting leaf overlap for node " << crag.id(n) << std::endl;
		_overlaps[n] = leafOverlaps(*volumes[n], groundTruth);

	} else {

		// get overlaps with all gt regions
		for (Crag::SubsetInArcIt a(crag.getSubsetGraph(), crag.toSubset(n)); a != lemon::INVALID; ++a) {

			Crag::Node child = crag.toRag(crag.getSubsetGraph().source(a));

			recurseOverlapLoss(crag, volumes, child, groundTruth);

			for (auto p : _overlaps[child])
				if (!_overlaps[n].count(p.first))
					_overlaps[n][p.first] = p.second;
				else
					_overlaps[n][p.first] += p.second;
		}
	}
}

std::map<int, int>
OverlapLoss::leafOverlaps(
		const ExplicitVolume<bool>& region,
		const ExplicitVolume<int>&  groundTruth) {

	std::map<int, int> overlaps;

	util::point<unsigned int, 3> offset =
			(region.getOffset() - groundTruth.getOffset())/
			groundTruth.getResolution();

	LOG_ALL(overlaplosslog) << "offset into ground-truth image: " << offset << std::endl;

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
OverlapLoss::foregroundNodeOverlapScore(
		const std::map<int, int>& overlaps) {

	double score = 0;

	// incorrectly merged pairs
	for (auto p : overlaps) {
		for (auto l : overlaps) {

			int label1 = p.first;
			int label2 = l.first;

			if (label1 >= label2)
				continue;

			if (label1 == 0)
				continue;

			double overlap1 = p.second;
			double overlap2 = l.second;

			LOG_ALL(overlaplosslog)
					<< "incorrectly merges " << label1
					<< " (" << overlap1 << " voxels) and " << label2
					<< " (" << overlap2 << " voxels)" << std::endl;
			LOG_ALL(overlaplosslog) << "+= " << overlap1*overlap2 << std::endl;

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

		LOG_ALL(overlaplosslog)
				<< "correctly merges " << label
				<< " (" << overlap << " voxels)" << std::endl;
		LOG_ALL(overlaplosslog) << "-= " << overlap*(overlap -1)/2 << std::endl;
	}

	return score;
}

double
OverlapLoss::foregroundEdgeOverlapScore(
		const std::map<int, int>& overlapsU,
		const std::map<int, int>& overlapsV) {

	double score = 0;

	for (auto p : overlapsU) {
		for (auto l : overlapsV) {

			int label1 = p.first;
			int label2 = l.first;

			if (label1 == 0 || label2 == 0)
				continue;

			double overlap1 = p.second;
			double overlap2 = l.second;

			// correctly merged
			if (label1 == label2) {

				score -= overlap1*overlap2;

				LOG_ALL(overlaplosslog)
						<< "correctly merges " << label1
						<< " (" << overlap1 << " voxels) and " << label2
						<< " (" << overlap2 << " voxels)" << std::endl;
				LOG_ALL(overlaplosslog) << "-= " << overlap1*overlap2 << std::endl;

			// incorrectly merged
			} else {

				score += overlap1*overlap2;

				LOG_ALL(overlaplosslog)
						<< "incorrectly merges " << label1
						<< " (" << overlap1 << " voxels) and " << label2
						<< " (" << overlap2 << " voxels)" << std::endl;
				LOG_ALL(overlaplosslog) << "+= " << overlap1*overlap2 << std::endl;
			}
		}
	}

	return score;
}

double
OverlapLoss::backgroundNodeOverlapScore(
		const std::map<int, int>& overlaps) {

	if (!overlaps.count(0))
		return 0;

	// overlap with background
	double overlap = overlaps.at(0);

	LOG_ALL(overlaplosslog)
			<< "overlaps with " << overlap
			<< " background voxels" << std::endl;
	LOG_ALL(overlaplosslog)
			<< "+= " << pow(overlap, 2) << std::endl;

	// reward is -overlap^2 of not selecting this node, i.e., punishment of 
	// overlap^2
	return pow(overlap, 2);
}
