#include <limits>
#include "RandLoss.h"
#include <util/Logger.h>
#include <util/ProgramOptions.h>

util::ProgramOption optionBalanceRandLoss(
		util::_module           = "loss.overlap",
		util::_long_name        = "balance",
		util::_description_text = "Compute the RAND loss only for leaf node and edges, "
		                          "and propagate the values upwards, such that each solution "
		                          "resulting in the same segmentation has the same loss. Note "
		                          "that this sacrifices approximation quality of the RAND "
		                          "loss, since even fewer transitive contributions are considered.");

util::ProgramOption optionRestrictRandLossToLeaves(
		util::_module           = "loss.overlap",
		util::_long_name        = "restrictToLeaves",
		util::_description_text = "Compute the RAND loss only for leaf node and edges, and "
		                          "set all other variable losses to a large value, such that "
		                          "they will not be picked.");

logger::LogChannel randlosslog("randlosslog", "[RandLoss] ");

RandLoss::RandLoss(
		const Crag&                crag,
		const CragVolumes&         volumes,
		const ExplicitVolume<int>& groundTruth) :
	Loss(crag),
	_overlaps(crag) {

	bool balance = optionBalanceRandLoss;
	bool restrictToLeaves = optionRestrictRandLossToLeaves;

	LOG_DEBUG(randlosslog) << "getting candidate overlaps..." << std::endl;

	// annotate all nodes with the max overlap area to any gt label
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		if (Crag::SubsetOutArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID)
			recurseRandLoss(crag, volumes, n, groundTruth);

	LOG_DEBUG(randlosslog) << "setting foreground RAND loss" << std::endl;

	// annotate nodes: loss is number of incorrectly merged pairs, minus number 
	// of correctly merged pairs
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		bool leafNode = (Crag::SubsetInArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID);

		if ((balance || restrictToLeaves) && !leafNode) {

			node[n] = (restrictToLeaves ? std::numeric_limits<double>::infinity() : 0);
			continue;
		}

		LOG_ALL(randlosslog)
				<< "getting RAND score for node " << crag.id(n) << std::endl;

		node[n] = foregroundNodeOverlapScore(_overlaps[n]) + backgroundNodeOverlapScore(_overlaps[n]);

		LOG_ALL(randlosslog)
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

		LOG_ALL(randlosslog)
				<< "getting foreground RAND score for "
				<< "edge (" << crag.id(crag.u(e))
				<< ", "     << crag.id(crag.v(e)) << ")"
				<< std::endl;

		edge[e] = foregroundEdgeOverlapScore(_overlaps[u], _overlaps[v]) + backgroundEdgeOverlapScore(_overlaps[u], _overlaps[v]);

		LOG_ALL(randlosslog)
				<< "edge (" << crag.id(crag.u(e))
				<< ", "     << crag.id(crag.v(e))
				<< "): "    << edge[e] << std::endl;
	}

	if (balance)
		propagateLeafLoss(crag);
}

void
RandLoss::recurseRandLoss(
		const Crag&                crag,
		const CragVolumes&         volumes,
		const Crag::Node&          n,
		const ExplicitVolume<int>& groundTruth) {

	bool leafNode = (Crag::SubsetInArcIt(crag.getSubsetGraph(), crag.toSubset(n)) == lemon::INVALID);
	if (leafNode) {

		LOG_ALL(randlosslog) << "getting leaf overlap for node " << crag.id(n) << std::endl;
		_overlaps[n] = leafOverlaps(*volumes[n], groundTruth);

	} else {

		// get overlaps with all gt regions
		for (Crag::SubsetInArcIt a(crag.getSubsetGraph(), crag.toSubset(n)); a != lemon::INVALID; ++a) {

			Crag::Node child = crag.toRag(crag.getSubsetGraph().source(a));

			recurseRandLoss(crag, volumes, child, groundTruth);

			for (auto p : _overlaps[child])
				if (!_overlaps[n].count(p.first))
					_overlaps[n][p.first] = p.second;
				else
					_overlaps[n][p.first] += p.second;
		}
	}
}

std::map<int, int>
RandLoss::leafOverlaps(
		const ExplicitVolume<bool>& region,
		const ExplicitVolume<int>&  groundTruth) {

	std::map<int, int> overlaps;

	util::point<unsigned int, 3> offset =
			(region.getOffset() - groundTruth.getOffset())/
			groundTruth.getResolution();

	LOG_ALL(randlosslog) << "offset into ground-truth image: " << offset << std::endl;

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
RandLoss::foregroundNodeOverlapScore(
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

			LOG_ALL(randlosslog)
					<< "incorrectly merges " << label1
					<< " (" << overlap1 << " voxels) and " << label2
					<< " (" << overlap2 << " voxels)" << std::endl;
			LOG_ALL(randlosslog) << "+= " << overlap1*overlap2 << std::endl;

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

		LOG_ALL(randlosslog)
				<< "correctly merges " << label
				<< " (" << overlap << " voxels)" << std::endl;
		LOG_ALL(randlosslog) << "-= " << overlap*(overlap -1)/2 << std::endl;
	}

	return score;
}

double
RandLoss::foregroundEdgeOverlapScore(
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

				LOG_ALL(randlosslog)
						<< "correctly merges " << label1
						<< " (" << overlap1 << " voxels) and " << label2
						<< " (" << overlap2 << " voxels)" << std::endl;
				LOG_ALL(randlosslog) << "-= " << overlap1*overlap2 << std::endl;

			// incorrectly merged
			} else {

				score += overlap1*overlap2;

				LOG_ALL(randlosslog)
						<< "incorrectly merges " << label1
						<< " (" << overlap1 << " voxels) and " << label2
						<< " (" << overlap2 << " voxels)" << std::endl;
				LOG_ALL(randlosslog) << "+= " << overlap1*overlap2 << std::endl;
			}
		}
	}

	return score;
}

double
RandLoss::backgroundNodeOverlapScore(
		const std::map<int, int>& overlaps) {

	if (!overlaps.count(0))
		return 0;

	// overlap with background
	double overlap = overlaps.at(0);

	LOG_ALL(randlosslog)
			<< "overlaps with " << overlap
			<< " background voxels" << std::endl;
	LOG_ALL(randlosslog)
			<< "+= " << pow(overlap, 2) << std::endl;

	// reward is -overlap^2 of not selecting this node, i.e., punishment of 
	// overlap^2
	return pow(overlap, 2);
}

double
RandLoss::backgroundEdgeOverlapScore(
		const std::map<int, int>& overlapsU,
		const std::map<int, int>& overlapsV) {

	if (!overlapsU.count(0) || !overlapsV.count(0))
		return 0;

	// overlap with background
	double overlapU = overlapsU.at(0);
	double overlapV = overlapsV.at(0);

	LOG_ALL(randlosslog)
			<< "adjacent nodes overlap with " << overlapU
			<< " and " << overlapV
			<< " background voxels" << std::endl;
	LOG_ALL(randlosslog)
			<< "+= " << 2*overlapU*overlapV << std::endl;

	return 2*overlapU*overlapV;
}

