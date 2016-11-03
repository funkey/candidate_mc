#ifndef CANDIDATE_MC_CRAG_CRAG_STACK_COMBINER_H__
#define CANDIDATE_MC_CRAG_CRAG_STACK_COMBINER_H__

#include <memory>
#include <vector>
#include "Crag.h"
#include "CragVolumes.h"

/**
 * Combines a stack of CRAGs(coming from a stack of images) into a single crag.
 */
class CragStackCombiner {

public:

	CragStackCombiner();

	/**
	 * Combine a stack of 2D CRAGS into one 3D CRAG. Adds hyperedges between the 
	 * candidates of two successive source CRAGS in the target CRAG.
	 *
	 * This delallocates the source CRAGs.
	 */
	void combine(
			std::vector<std::unique_ptr<Crag>>&        sourcesCrags,
			std::vector<std::unique_ptr<CragVolumes>>& sourcesVolumes,
			Crag&                                      targetCrag,
			CragVolumes&                               targetVolumes);

private:

	std::map<Crag::CragNode, Crag::CragNode> copyNodes(
			unsigned int       z,
			const Crag&        source,
			Crag&              target);

	void copyVolumes(
			const CragVolumes& sourceVolumes,
			CragVolumes& targetVolumes,
			const std::map<Crag::CragNode, Crag::CragNode>& sourceTargetNodeMap);

	std::vector<std::pair<Crag::CragNode, Crag::CragNode>> findLinks(const Crag& a, const Crag& b);

	std::vector<std::pair<Crag::CragNode, Crag::CragNode>> findLinks(
			const Crag&        cragA,
			const CragVolumes& volsA,
			const Crag&        cragB,
			const CragVolumes& volsB);

	double _maxHausdorffDistance;
	double _maxBbDistance;

	bool _requireBbOverlap;

	std::map<Crag::CragNode, Crag::CragNode> _prevNodeMap;
	std::map<Crag::CragNode, Crag::CragNode> _nextNodeMap;

	std::vector<Crag::CragNode> _noAssignmentNodes;
};

#endif // CANDIDATE_MC_CRAG_CRAG_STACK_COMBINER_H__

