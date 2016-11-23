#include <inference/CragSolverFactory.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include "BestEffort.h"

logger::LogChannel bestEffortlog("bestEffortlog", "[BestEffort] ");

util::ProgramOption optionFullBestEffort(
		util::_long_name        = "fullBestEffort",
		util::_description_text = "When finding the best-effort using the assignment heuristic, include all candidates and all adjacency "
								"edges that produce the same segmentation. I.e., if a candidate was selected to be part of the best-effort, "
								"all its children will be selected as well (and the edges connecting them).");

util::ProgramOption optionBackgroundOverlapWeight(
		util::_long_name        = "backgroundOverlapWeight",
		util::_description_text = "The weight of background voxels for the computation of the best-effort. A value smaller than 1 means "
		                          "that a supervoxel can be assigned to a ground-truth region even though it overlaps with more than 50% "
		                          "with background.",
		util::_default_value    = 1);

util::ProgramOption optionMajorityOverlap(
		util::_module           = "best-effort",
		util::_long_name        = "majorityOverlap",
		util::_description_text = "Switch to an alternative strategy to find the best-effort solution. If set, the largest candidate that "
		                          "has a majority overlap with a ground-truth region will be selected and assigned to this region. If none "
		                          "of the candidates along a path has a majority overlap, the leaf node is selected and assigned to the ground-"
		                          "truth region with maximal overlap. If two adjacent candidates are selected and assigned to the same ground-"
		                          "truth region, the adjacency edge is also selected. If this option is not set, the largest candidate that "
		                          "has leaf nodes that are all assigned to the same ground-truth region is selected and assigned to this region.");

BestEffort::BestEffort(
		const Crag&                   crag,
		const CragVolumes&            volumes,
		const Costs&                  costs,
		const CragSolver::Parameters& params) :
	CragSolution(crag),
	_fullBestEffort(optionFullBestEffort),
	_bgOverlapWeight(optionBackgroundOverlapWeight) {

	std::unique_ptr<CragSolver> solver(CragSolverFactory::createSolver(crag, volumes, params));
	solver->setCosts(costs);
	solver->solve(*this);
}

BestEffort::BestEffort(
		const Crag&                   crag,
		const CragVolumes&            volumes,
		const ExplicitVolume<int>&    groundTruth) :
	CragSolution(crag),
	_fullBestEffort(optionFullBestEffort),
	_bgOverlapWeight(optionBackgroundOverlapWeight){

	for (Crag::CragNode n : crag.nodes())
		setSelected(n, false);

	for (Crag::CragEdge e : crag.edges())
		setSelected(e, false);

	// assign each candidate to the ground-truth region with maximal overlap (this does not select the candidates, yet)

	Crag::NodeMap<std::map<int, int>> overlaps(crag);
	getGroundTruthOverlaps(crag, volumes, groundTruth, overlaps);

	Crag::NodeMap<int> gtAssignments(crag);
	getGroundTruthAssignments(crag, overlaps, gtAssignments);

	// recursively find the largest candidates assigned to only one ground-truth 
	// region
	if (optionMajorityOverlap)
		findMajorityOverlapCandidates(crag, overlaps, gtAssignments);
	else
		findConcordantLeafNodeCandidates(crag, gtAssignments);

	// find all edges connecting switched on candidates assigned to the same 
	// ground-truth region
	for (Crag::CragEdge e : crag.edges()) {

	    if (crag.type(e) == Crag::AssignmentEdge)
	        continue;

		Crag::CragNode u = crag.u(e);
		Crag::CragNode v = crag.v(e);

		if (!selected(u) || !selected(v))
			continue;

		if (gtAssignments[u] != 0 && gtAssignments[u] == gtAssignments[v])
			setSelected(e, true);
	}

    // For the Assignment Model, select the assignment nodes and edges
    selectAssignments( crag, volumes, groundTruth, gtAssignments, overlaps );

}

void
BestEffort::getGroundTruthOverlaps(
		const Crag&                        crag,
		const CragVolumes&                 volumes,
		const ExplicitVolume<int>&         groundTruth,
		Crag::NodeMap<std::map<int, int>>& overlaps) {

	for (Crag::CragNode n : crag.nodes()) {

		if (crag.type(n) == Crag::NoAssignmentNode)
			continue;

		const CragVolume& region = *volumes[n];

		util::point<unsigned int, 3> offset =
				(region.getOffset() - groundTruth.getOffset())/
				groundTruth.getResolution();

		for (unsigned int z = 0; z < region.getDiscreteBoundingBox().depth();  z++)
		for (unsigned int y = 0; y < region.getDiscreteBoundingBox().height(); y++)
		for (unsigned int x = 0; x < region.getDiscreteBoundingBox().width();  x++) {

			if (!region.data()(x, y, z))
				continue;

			int gtLabel = groundTruth[offset + util::point<unsigned int, 3>(x, y, z)];

			overlaps[n][gtLabel]++;
		}
	}
}

void
BestEffort::getGroundTruthAssignments(
		const Crag&                              crag,
		const Crag::NodeMap<std::map<int, int>>& overlaps,
		Crag::NodeMap<int>&                      gtAssignments) {

	for (Crag::CragNode i : crag.nodes()) {

		if (crag.type(i) == Crag::NoAssignmentNode)
			continue;

		// find most overlapping ground truth region
		double maxOverlap = 0;
		int bestGtLabel = 0;

		for (auto& p : overlaps[i]) {

			int gtLabel = p.first;
			double overlap = p.second;

			if (gtLabel == 0)
				overlap *= _bgOverlapWeight;

			if (overlap > maxOverlap) {

				maxOverlap = overlap;
				bestGtLabel = gtLabel;
			}
		}

		gtAssignments[i] = bestGtLabel;
	}
}

void
BestEffort::getLeafAssignments(
		const Crag&                   crag,
		Crag::CragNode                n,
		const Crag::NodeMap<int>&     gtAssignments,
		Crag::NodeMap<std::set<int>>& leafAssignments) {

	leafAssignments[n].clear();

	// add all our children's assignments
	for (Crag::CragArc childArc : crag.inArcs(n)) {
		getLeafAssignments(crag, childArc.source(), gtAssignments, leafAssignments);
		leafAssignments[n].insert(leafAssignments[childArc.source()].begin(), leafAssignments[childArc.source()].end());
	}

	// add our own assignment
	leafAssignments[n].insert(gtAssignments[n]);
}

void
BestEffort::findMajorityOverlapCandidates(
		const Crag&                              crag,
		const Crag::NodeMap<std::map<int, int>>& overlaps,
		const Crag::NodeMap<int>&                gtAssignments) {
  
	for (Crag::CragNode n : crag.nodes())
	{
		if (crag.type(n) == Crag::NoAssignmentNode ||
			crag.type(n) == Crag::AssignmentNode)
			continue;

		if (crag.isRootNode(n))
			labelMajorityOverlapCandidate(crag, n, overlaps, gtAssignments);
	}
}

void
BestEffort::findConcordantLeafNodeCandidates(
        const Crag&                              crag,
        const Crag::NodeMap<int>&                gtAssignments) {

    Crag::NodeMap<std::set<int>> leafAssignments(crag);

    for (Crag::CragNode n : crag.nodes())
        if (crag.isRootNode(n))
            getLeafAssignments(crag, n, gtAssignments, leafAssignments);

    for (Crag::CragNode n : crag.nodes())
        if (crag.isRootNode(n))
                labelSingleAssignmentCandidate(crag, n, leafAssignments);
}


void
BestEffort::labelMajorityOverlapCandidate(
		const Crag&                              crag,
		const Crag::CragNode&                    n,
		const Crag::NodeMap<std::map<int, int>>& overlaps,
		const Crag::NodeMap<int>&                gtAssignments) {

	double maxOverlap = overlaps[n].at(gtAssignments[n]);

	if (gtAssignments[n] == 0)
		maxOverlap *= _bgOverlapWeight;

	double totalOverlap = 0;
	for (auto& p : overlaps[n])
		totalOverlap += ((double)p.second)*(p.first == 0 ? _bgOverlapWeight : 1.0);
 
	if (crag.isLeafNode(n) || maxOverlap/totalOverlap > 0.5) {

		setSelected(n, gtAssignments[n] != 0);

		// for the full best-effort, we continue going down
		if (!_fullBestEffort)
			return;
	}

	for (Crag::CragArc childArc : crag.inArcs(n))
		labelMajorityOverlapCandidate(crag, childArc.source(), overlaps, gtAssignments);
}

void
BestEffort::labelSingleAssignmentCandidate(
		const Crag&                         crag,
		Crag::CragNode                      n,
		const Crag::NodeMap<std::set<int>>& leafAssignments) {

    if (leafAssignments[n].size() == 1 && (*leafAssignments[n].begin()) != 0) {

        if(crag.type(n) != Crag::AssignmentNode && crag.type(n) != Crag::NoAssignmentNode )
        {
            setSelected(n, true);

            // for the full best-effort, we continue going down
            if (!_fullBestEffort)
                return;
        }
	}

	for (Crag::CragArc childArc : crag.inArcs(n))
	    labelSingleAssignmentCandidate(crag, childArc.source(), leafAssignments);
}

void BestEffort::selectAssignments(
		const Crag&                 crag,
        const CragVolumes&          volumes,
        const ExplicitVolume<int>&  groundTruth,
		Crag::NodeMap<int>&         gtAssignments,
		Crag::NodeMap<std::map<int, int>>& overlaps)
{

    // for each slice node, if a parent is selected, unselected all children
    for (Crag::CragNode n : crag.nodes()) {

        if (crag.type(n) != Crag::SliceNode)
            continue;

        if (selected(n))
            unselectChildren(crag, n);
    }

    // For all assignment nodes, check if it links two selected candidates with the same label
    for (Crag::CragNode n : crag.nodes()) {

        if (crag.type(n) != Crag::AssignmentNode)
            continue;

        int label = -1;
        Crag::CragNode previousChild;
        for (Crag::CragEdge edge : crag.adjEdges(n)) {

            Crag::CragNode child = edge.u();

            // If the candidate is not select, go to the next assignmentoNode
            if (!selected(child))
                break;

            if (label == -1) {
                label = gtAssignments[child];
                previousChild = child;
            }
            else if (label == gtAssignments[child] && label == gtAssignments[n]) {
                // Select the assignment node with the same label
                setSelected(n, true);
                LOG_ALL(bestEffortlog) << "\tselecting assignment node " <<  crag.id(n) << " with label: " << gtAssignments[n] << std::endl;
            }
        }
    }

    // For all assignmentEdges, select that one who has two selected nodes
    for (Crag::CragEdge e : crag.edges()) {

        if(crag.type(e) != Crag::AssignmentEdge)
            continue;

        Crag::CragNode u = crag.u(e);
        Crag::CragNode v = crag.v(e);

        if (!selected(u) || !selected(v))
            continue;

        if (gtAssignments[u] != 0 && gtAssignments[u] == gtAssignments[v])
        {
            setSelected(e, true);
            LOG_ALL(bestEffortlog) << "\tselecting edge linking node " <<  crag.id(u) << " and " << crag.id(v) << std::endl;
        }
    }

    checkConstraint( crag, volumes, groundTruth, gtAssignments, overlaps );

    selectNoAssignmentEdges( crag, volumes, groundTruth );

//#ifdef DEBUG
    LOG_DEBUG(bestEffortlog) << "\tChecking results: selected edges for each selected slice node::" <<  std::endl;
    // Check if there is a selected candidate with more than two assignment edges selected
    bool OK = true;
    for (Crag::CragNode n : crag.nodes()) {

        if (crag.type(n) != Crag::SliceNode)
            continue;

        if (!selected(n))
            continue;

        int assignmentSelected = 0;
        for (Crag::CragEdge edge : crag.adjEdges(n))
            if (crag.type(edge) == Crag::AssignmentEdge || crag.type(edge) == Crag::NoAssignmentEdge)
                if(selected(edge))
                    assignmentSelected++;

        if (assignmentSelected == 0){
            LOG_DEBUG(bestEffortlog) << "\tslice node without assignmentEdges selected - id: " << crag.id(n) <<  std::endl;
            OK = false;
        }else if (assignmentSelected == 1){
            LOG_DEBUG(bestEffortlog) << "\tslice node with one assignmentEdge selected - id: " << crag.id(n) <<  std::endl;
            OK = false;
        }else if (assignmentSelected > 2){
            LOG_DEBUG(bestEffortlog) << "\tslice node with more than two assignmentEdge selected - id: " << crag.id(n) <<  std::endl;
            OK = false;
        }
    }
    if(OK)
        LOG_DEBUG(bestEffortlog) << "\tOK" <<  std::endl;

//#endif
}

void BestEffort::unselectChildren(
        const Crag&    crag,
        Crag::CragNode n)
{
    for (Crag::CragArc arc : crag.inArcs(n))
    {
        setSelected(arc.source(), false);
        unselectChildren( crag, arc.source() );
    }
}

void BestEffort::checkConstraint(
        const Crag&                 crag,
        const CragVolumes&          volumes,
        const ExplicitVolume<int>&  groundTruth,
        Crag::NodeMap<int>&         gtAssignments,
        Crag::NodeMap<std::map<int, int>>& overlaps)
{
    // For all selected sliceNodes, check if they have more than one assignment node selected per section
    for (Crag::CragNode n : crag.nodes()) {
        if (crag.type(n) != Crag::SliceNode)
            continue;

        if (!selected(n))
            continue;

        int assignmentSelected = 0;
        int section = -1;
        Crag::CragNode previous;
        for (Crag::CragEdge edge : crag.adjEdges(n)) {

            if (crag.type(edge) != Crag::AssignmentEdge)
                continue;

            Crag::CragNode child = edge.v();

            if (!selected(edge))
                continue;

            // Identify the assignment node section
            const CragVolume& region = *volumes[child];

            util::point<unsigned int, 3> offset = (region.getOffset()
                    - groundTruth.getOffset()) / groundTruth.getResolution();

            // If the edge is selected, so is the assignmentNode
            assignmentSelected++;
            previous = child;

            if (section == -1)
                section = offset.z();

            if (assignmentSelected > 1) {

                if (section == offset.z()) {
                    assignmentSelected--;
                    // keep selected only the one with the most overlaping gt area
                    Crag::CragNode removed =
                            (overlaps[previous] > overlaps[child]) ?
                                    child : previous;

                    setSelected(removed, false);
                    LOG_ALL(bestEffortlog) << "\tunselecting assignment node: "
                            << crag.id(child) << std::endl;
                    // unselect all edges from the assignment node
                    for (Crag::CragEdge e : crag.adjEdges(removed)) {
                        setSelected(e, false);
                        LOG_ALL(bestEffortlog)
                                << "\tunselecting edges between: "
                                << crag.id(crag.u(e)) << " and "
                                << crag.id(crag.v(e)) << std::endl;
                    }
                } else {
                    section = offset.z();
                    assignmentSelected--;
                }
            }
        }
    }
}

void BestEffort::selectNoAssignmentEdges(
        const Crag&                 crag,
        const CragVolumes&          volumes,
        const ExplicitVolume<int>&  groundTruth){

    // Count sections
    int sections = -1;
    for (Crag::CragNode n : crag.nodes())
        if (crag.type(n) == Crag::NoAssignmentNode)
            sections++;

    // Check if there is a selected candidate missing assignment
    for (Crag::CragNode n : crag.nodes()) {

        if (crag.type(n) != Crag::SliceNode)
            continue;

        if (!selected(n))
            continue;

        int assignmentSelected = 0;
        for (Crag::CragEdge edge : crag.adjEdges(n))
            if (crag.type(edge) == Crag::AssignmentEdge)
                if (selected(edge))
                    assignmentSelected++;

        // Check in which section the selected node is
        const CragVolume& region = *volumes[n];

        util::point<unsigned int, 3> offset = (region.getOffset()
                - groundTruth.getOffset()) / groundTruth.getResolution();

        // No one assignment node selected, should select the noAssignmentEdge
        if (assignmentSelected == 0) {

            for (Crag::CragEdge edge : crag.adjEdges(n)) {
                if (crag.type(edge) != Crag::NoAssignmentEdge)
                    continue;

                // select the noAssignmentEdge before the section
                if ((crag.id(edge.v()) == offset.z()))
                    setSelected(edge, true);

                // selecte the noAssignmentEdge after the section
                if ((crag.id(edge.v()) == offset.z() + 1))
                    setSelected(edge, true);
            }
        } else if (assignmentSelected == 1) {
            int section = 0;
            // First or last section
            if (offset.z() == 0 || (offset.z() == (sections - 1))) {
                section = (offset.z() == 0) ? 0 : sections;
            } else {
                // Search for the position of the assignmentNode selected
                util::point<unsigned int, 3> assignmentOffset;
                for (Crag::CragEdge edge : crag.adjEdges(n)) {
                    if (crag.type(edge.v()) != Crag::AssignmentNode)
                        continue;
                    if (!selected(edge.v()))
                        continue;

                    const CragVolume& region = *volumes[edge.v()];

                    assignmentOffset = (region.getOffset()
                            - groundTruth.getOffset())
                            / groundTruth.getResolution();
                }

                section =
                        (offset.z() == assignmentOffset.z()) ?
                                offset.z() : offset.z() + 1;
            }

            for (Crag::CragEdge edge : crag.adjEdges(n))
                if (crag.type(edge) == Crag::NoAssignmentEdge)
                    if (crag.id(edge.v()) == section)
                        setSelected(edge, true);
        }
    }
}

