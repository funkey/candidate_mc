#include <features/HausdorffDistance.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/assert.h>
#include <util/exceptions.h>
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
		                          "considered adjacent. For that, the maximal "
		                          "value of the two directions are taken as "
		                          "distance.",
		util::_default_value    = 50);

CragStackCombiner::CragStackCombiner() :
	_maxDistance(optionMaxZLinkHausdorffDistance),
	_requireBbOverlap(optionRequireBoundingBoxOverlap) {}

void
CragStackCombiner::combine(
		std::vector<std::unique_ptr<Crag>>&        sourcesCrags,
		std::vector<std::unique_ptr<CragVolumes>>& sourcesVolumes,
		Crag&                                      targetCrag,
		CragVolumes&                               targetVolumes) {

	UTIL_ASSERT_REL(sourcesCrags.size(), ==, sourcesVolumes.size());

	if (sourcesCrags.size() == 0)
		return;

	LOG_USER(cragstackcombinerlog)
			<< "combining CRAGs, "
			<< (_requireBbOverlap ? "" : " do not ")
			<< "require bounding box overlap"
			<< std::endl;

	_prevNodeMap.clear();
	_nextNodeMap.clear();

	// get the resolution of source volumes
	util::point<float, 3> res;
	for (unsigned int z = 0; z < sourcesCrags.size(); z++)
		if (sourcesCrags[z]->nodes().size() > 0) {

			res = (*sourcesVolumes[z])[*sourcesCrags[z]->nodes().begin()]->getResolution();
			break;
		}

	if (res.isZero())
		UTIL_THROW_EXCEPTION(
				UsageError,
				"all provided CRAGs are empty");

	// add one NoAssignmentNode between each pair of crags, and before first and 
	// after last section
	_noAssignmentNodes.clear();
	for (unsigned int z = 0; z <= sourcesCrags.size(); z++) {

		Crag::CragNode n = targetCrag.addNode(Crag::NoAssignmentNode);
		_noAssignmentNodes.push_back(n);

		LOG_ALL(cragstackcombinerlog) << "added no-assignment node with id " << targetCrag.id(n) << std::endl;

		// set a dummy 1x1x1 volume
		std::shared_ptr<CragVolume> dummy = std::make_shared<CragVolume>(1, 1, 1);
		dummy->data() = 1;
		dummy->setOffset(
				sourcesVolumes[0]->getBoundingBox().min().x(),
				sourcesVolumes[0]->getBoundingBox().min().y(),
				sourcesVolumes[0]->getBoundingBox().min().z() + (z - 0.5)*res.z());
		dummy->setResolution(res);

		LOG_ALL(cragstackcombinerlog) << "bb of no-assignment node is " << dummy->getBoundingBox() << std::endl;

		targetVolumes.setVolume(n, dummy);
	}

	unsigned int nodesAdded = 0;
	for (unsigned int z = 1; z < sourcesCrags.size(); z++) {

		LOG_USER(cragstackcombinerlog) << "linking CRAG " << (z-1) << " and " << z << std::endl;

		std::vector<std::pair<Crag::CragNode, Crag::CragNode>> links =
				findLinks(
						*sourcesCrags[z-1],
						*sourcesVolumes[z-1],
						*sourcesCrags[z],
						*sourcesVolumes[z]);

		if (z == 1)
			_prevNodeMap = copyNodes(0, *sourcesCrags[0], targetCrag);
		else
			_prevNodeMap = _nextNodeMap;

		_nextNodeMap = copyNodes(z, *sourcesCrags[z], targetCrag);

		for (const auto& pair : links) {

			Crag::CragNode assignment = targetCrag.addNode(Crag::AssignmentNode);

			targetCrag.addAdjacencyEdge(_prevNodeMap[pair.first], assignment, Crag::AssignmentEdge);
			targetCrag.addAdjacencyEdge(_nextNodeMap[pair.second], assignment, Crag::AssignmentEdge);
			targetCrag.addSubsetArc(_prevNodeMap[pair.first], assignment);
			targetCrag.addSubsetArc(_nextNodeMap[pair.second], assignment);
		}

		nodesAdded += links.size();

		copyVolumes(*sourcesVolumes[z-1], targetVolumes, _prevNodeMap);
		if (z == sourcesCrags.size() - 1)
			copyVolumes(*sourcesVolumes[z], targetVolumes, _nextNodeMap);

		// from now on, we don't need sourcesVolumes[z-1] and sourcesCrags[z-1] 
		// anymore. free some memory.
		sourcesCrags[z-1].reset();
		sourcesVolumes[z-1].reset();
	}

	// clear the sources
	sourcesCrags.clear();
	sourcesVolumes.clear();

	LOG_USER(cragstackcombinerlog) << "added " << nodesAdded << " link nodes" << std::endl;
}

std::map<Crag::CragNode, Crag::CragNode>
CragStackCombiner::copyNodes(
		unsigned int       z,
		const Crag&        source,
		Crag&              target) {

	std::map<Crag::CragNode, Crag::CragNode> nodeMap;

	// copy nodes
	for (Crag::CragNode i : source.nodes()) {

		Crag::CragNode n = target.addNode(Crag::SliceNode);

		nodeMap[i] = n;

		// add adjacencies to NoAssignmentNodes befor and after
		target.addAdjacencyEdge(n, _noAssignmentNodes[z], Crag::NoAssignmentEdge);
		target.addAdjacencyEdge(n, _noAssignmentNodes[z+1], Crag::NoAssignmentEdge);
	}

	// copy adjacencies
	for (Crag::CragEdge e : source.edges()) {

		Crag::CragNode u = nodeMap[e.u()];
		Crag::CragNode v = nodeMap[e.v()];

		target.addAdjacencyEdge(u, v, source.type(e));
	}

	// copy subset relations
	for (Crag::CragArc a : source.arcs()) {

		Crag::CragNode s = nodeMap[a.source()];
		Crag::CragNode t = nodeMap[a.target()];

		target.addSubsetArc(s, t);
	}

	return nodeMap;
}

void
CragStackCombiner::copyVolumes(
		const CragVolumes& sourceVolumes,
		CragVolumes& targetVolumes,
		const std::map<Crag::CragNode, Crag::CragNode>& sourceTargetNodeMap) {

	// copy nodes
	for (Crag::CragNode i : sourceVolumes.getCrag().nodes()) {

		// copy only leaf nodes
		if (!sourceVolumes.getCrag().isLeafNode(i))
			continue;

		Crag::CragNode n = sourceTargetNodeMap.at(i);
		targetVolumes.setVolume(n, sourceVolumes[i]);
	}
}

std::vector<std::pair<Crag::CragNode, Crag::CragNode>>
CragStackCombiner::findLinks(
		const Crag&        cragA,
		const CragVolumes& volsA,
		const Crag&        cragB,
		const CragVolumes& volsB) {

	UTIL_TIME_METHOD;

	std::vector<std::pair<Crag::CragNode, Crag::CragNode>> links;

	if (cragA.nodes().size() == 0 || cragB.nodes().size() == 0)
		return links;

	// make sure the padding for the distance map is at least on pixel more than 
	// then the cut-off at max distance
	double maxResolution = std::max(
			volsA[*cragA.nodes().begin()]->getResolutionX(),
			volsA[*cragA.nodes().begin()]->getResolutionY());
	HausdorffDistance hausdorff(_maxDistance + maxResolution);

	for (Crag::CragNode i : cragA.nodes()) {

		for (Crag::CragNode j : cragB.nodes()) {

			LOG_ALL(cragstackcombinerlog)
					<< "check linking of nodes " << cragA.id(i)
					<< " and " << cragB.id(j) << std::endl;

			if (_requireBbOverlap) {

				util::box<float, 2> bb_i = volsA.getBoundingBox(i).project<2>();
				util::box<float, 2> bb_j = volsB.getBoundingBox(j).project<2>();

				LOG_ALL(cragstackcombinerlog)
						<< "bounding boxes are " << bb_i << " and " << bb_j << std::endl;

				if (!bb_i.intersects(bb_j))
					continue;

				LOG_ALL(cragstackcombinerlog)
						<< "bounding boxes overlap" << std::endl;
			}

			double i_j, j_i;
			hausdorff(*volsA[i], *volsB[j], i_j, j_i);

			LOG_ALL(cragstackcombinerlog)
					<< "Hausdorff distances are: " << i_j << " and " << j_i << std::endl;

			double distance = std::max(i_j, j_i);

			if (distance <= _maxDistance) {

				LOG_ALL(cragstackcombinerlog)
						<< "smaller than " << _maxDistance
						<< ", adding link" << std::endl;

				links.push_back(std::make_pair(i, j));
			}
		}
	}

	return links;
}
