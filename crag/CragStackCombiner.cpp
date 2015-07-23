#include <features/HausdorffDistance.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/assert.h>
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
CragStackCombiner::combine(
		const std::vector<Crag>&        sourcesCrags,
		const std::vector<CragVolumes>& sourcesVolumes,
		Crag&                           targetCrag,
		CragVolumes&                    targetVolumes) {

	UTIL_ASSERT_REL(sourcesCrags.size(), ==, sourcesVolumes.size());

	LOG_USER(cragstackcombinerlog)
			<< "combining CRAGs, "
			<< (_requireBbOverlap ? "" : " do not ")
			<< "require bounding box overlap"
			<< std::endl;

	std::map<Crag::CragNode, Crag::CragNode> prevNodeMap;
	std::map<Crag::CragNode, Crag::CragNode> nextNodeMap;

	for (unsigned int z = 1; z < sourcesCrags.size(); z++) {

		LOG_USER(cragstackcombinerlog) << "linking CRAG " << (z-1) << " and " << z << std::endl;

		if (z == 1)
			prevNodeMap = copyNodes(sourcesCrags[0], sourcesVolumes[0], targetCrag, targetVolumes);
		else
			prevNodeMap = nextNodeMap;

		nextNodeMap = copyNodes(sourcesCrags[z], sourcesVolumes[z], targetCrag, targetVolumes);

		std::vector<std::pair<Crag::CragNode, Crag::CragNode>> links =
				findLinks(
						sourcesCrags[z-1],
						sourcesVolumes[z-1],
						sourcesCrags[z],
						sourcesVolumes[z]);

		for (const auto& pair : links)
			targetCrag.addAdjacencyEdge(
					prevNodeMap[pair.first],
					nextNodeMap[pair.second]);
	}
}

std::map<Crag::CragNode, Crag::CragNode>
CragStackCombiner::copyNodes(
		const Crag&        source,
		const CragVolumes& sourceVolumes,
		Crag&              target,
		CragVolumes&       targetVolumes) {

	std::map<Crag::CragNode, Crag::CragNode> nodeMap;

	for (Crag::CragNode i : source.nodes()) {

		Crag::CragNode n = target.addNode();

		if (source.isLeafNode(i))
			targetVolumes.setLeafNodeVolume(n, sourceVolumes[i]);

		nodeMap[i] = n;
	}

	for (Crag::CragEdge e : source.edges()) {

		Crag::CragNode u = nodeMap[e.u()];
		Crag::CragNode v = nodeMap[e.v()];

		target.addAdjacencyEdge(u, v);
	}

	for (Crag::CragArc a : source.arcs()) {

		Crag::CragNode s = nodeMap[a.source()];
		Crag::CragNode t = nodeMap[a.target()];

		target.addSubsetArc(s, t);
	}

	return nodeMap;
}

std::vector<std::pair<Crag::CragNode, Crag::CragNode>>
CragStackCombiner::findLinks(
		const Crag&        cragA,
		const CragVolumes& volsA,
		const Crag&        cragB,
		const CragVolumes& volsB) {

	UTIL_TIME_METHOD;

	HausdorffDistance hausdorff(volsA, volsB, 100);

	std::vector<std::pair<Crag::CragNode, Crag::CragNode>> links;

	for (Crag::CragNode i : cragA.nodes()) {

		LOG_DEBUG(cragstackcombinerlog) << "checking for links of node " << cragA.id(i) << std::endl;

		for (Crag::CragNode j: cragB.nodes()) {

			if (_requireBbOverlap) {

				util::box<float, 2> bb_i = volsA[i]->getBoundingBox().project<2>();
				util::box<float, 2> bb_j = volsB[j]->getBoundingBox().project<2>();

				if (!bb_i.intersects(bb_j))
					continue;
			}

			double i_j, j_i;
			hausdorff(i, j, i_j, j_i);

			double distance = std::min(i_j, j_i);

			if (distance <= _maxDistance)
				links.push_back(std::make_pair(i, j));
		}
	}

	return links;
}
