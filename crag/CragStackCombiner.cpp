#include <features/HausdorffDistance.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include <util/timing.h>
#include "CragStackCombiner.h"

logger::LogChannel cragstackcombinerlog("cragstackcombinerlog", "[CragStackCombiner] ");

util::ProgramOption optionRequireBoundingBoxOverlap(
		util::_long_name        = "requireBoundingBoxOverlap",
		util::_module           = "crag.combine",
		util::_description_text = "To consider two superpixels in subsequent z-secitons "
		                          "to be linked, require their bounding boxes to overlap. "
		                          "Default is true.",
		util::_default_value    = true);

util::ProgramOption optionMaxZLinkHausdorffDistance(
		util::_long_name        = "maxZLinkHausdorffDistance",
		util::_module           = "crag.combine",
		util::_description_text = "The maximal Hausdorff distance between two "
		                          "superpixels in subsequent z-section to be "
		                          "considered adjacent. For that, the minimal "
		                          "value of the two directions are taken as "
		                          "distance.",
		util::_default_value    = 50);

CragStackCombiner::CragStackCombiner() :
	_maxDistance(optionMaxZLinkHausdorffDistance),
	_requireBbOverlap(optionRequireBoundingBoxOverlap) {}

void
CragStackCombiner::combine(const std::vector<Crag>& crags, Crag& crag) {

	LOG_USER(cragstackcombinerlog)
			<< "combining CRAGs, "
			<< (_requireBbOverlap ? "" : " do not ")
			<< "require bounding box overlap"
			<< std::endl;

	std::map<Crag::Node, Crag::Node> prevNodeMap;
	std::map<Crag::Node, Crag::Node> nextNodeMap;

	for (unsigned int z = 1; z < crags.size(); z++) {

		LOG_USER(cragstackcombinerlog) << "linking CRAG " << (z-1) << " and " << z << std::endl;

		if (z == 1)
			prevNodeMap = copyNodes(crags[0], crag);
		else
			prevNodeMap = nextNodeMap;

		nextNodeMap = copyNodes(crags[z], crag);

		std::vector<std::pair<Crag::Node, Crag::Node>> links = findLinks(crags[z-1], crags[z]);

		for (const auto& pair : links)
			crag.addAdjacencyEdge(
					prevNodeMap[pair.first],
					nextNodeMap[pair.second]);
	}
}

std::map<Crag::Node, Crag::Node>
CragStackCombiner::copyNodes(const Crag& source, Crag& target) {

	std::map<Crag::Node, Crag::Node> nodeMap;

	for (Crag::NodeIt i(source); i != lemon::INVALID; ++i) {

		Crag::Node n = target.addNode();

		ExplicitVolume<unsigned char> volume = source.getVolume(i);

		if (!volume.getBoundingBox().isZero())
			target.getVolume(n) = volume;

		nodeMap[i] = n;
	}

	for (Crag::EdgeIt e(source); e != lemon::INVALID; ++e) {

		Crag::Node u = nodeMap[source.u(e)];
		Crag::Node v = nodeMap[source.v(e)];

		target.addAdjacencyEdge(u, v);
	}

	for (Crag::SubsetArcIt a(source); a != lemon::INVALID; ++a) {

		Crag::Node s = nodeMap[source.toRag(source.getSubsetGraph().source(a))];
		Crag::Node t = nodeMap[source.toRag(source.getSubsetGraph().target(a))];

		target.addSubsetArc(s, t);
	}

	return nodeMap;
}

std::vector<std::pair<Crag::Node, Crag::Node>>
CragStackCombiner::findLinks(const Crag& a, const Crag& b) {

	UTIL_TIME_METHOD;

	HausdorffDistance hausdorff(a, b, 100);

	std::vector<std::pair<Crag::Node, Crag::Node>> links;

	for (Crag::NodeIt i(a); i != lemon::INVALID; ++i) {

		LOG_DEBUG(cragstackcombinerlog) << "checking for links of node " << a.id(i) << std::endl;

		for (Crag::NodeIt j(b); j != lemon::INVALID; ++j) {

			if (_requireBbOverlap) {

				util::box<float, 2> bb_i = a.getVolume(i).getBoundingBox().project<2>();
				util::box<float, 2> bb_j = b.getVolume(j).getBoundingBox().project<2>();

				if (!bb_i.intersects(bb_j))
					continue;
			}

			double i_j, j_i;
			hausdorff.distance(i, j, i_j, j_i);

			double distance = std::min(i_j, j_i);

			if (distance <= _maxDistance)
				links.push_back(std::make_pair(i, j));
		}
	}

	return links;
}
