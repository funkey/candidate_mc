#ifndef CANDIDATE_MC_CRAG_CRAG_STACK_COMBINER_H__
#define CANDIDATE_MC_CRAG_CRAG_STACK_COMBINER_H__

#include <vector>
#include "Crag.h"
#include "CragVolumes.h"

/**
 * Combines a stack of CRAGs(coming from a stack of images) into a single crag.
 */
class CragStackCombiner {

public:

	CragStackCombiner();

	void combine(
			const std::vector<Crag>&        sourcesCrags,
			const std::vector<CragVolumes>& sourcesVolumes,
			Crag&                           targetCrag,
			CragVolumes&                    targetVolumes);

private:

	std::map<Crag::CragNode, Crag::CragNode> copyNodes(
			const Crag&        source,
			const CragVolumes& sourceVolumes,
			Crag&              target,
			CragVolumes&       targetVolumes);

	std::vector<std::pair<Crag::CragNode, Crag::CragNode>> findLinks(const Crag& a, const Crag& b);

	std::vector<std::pair<Crag::CragNode, Crag::CragNode>> findLinks(
			const Crag&        cragA,
			const CragVolumes& volsA,
			const Crag&        cragB,
			const CragVolumes& volsB);

	double _maxDistance;

	bool _requireBbOverlap;

	std::map<Crag::CragNode, Crag::CragNode> _prevNodeMap;
	std::map<Crag::CragNode, Crag::CragNode> _nextNodeMap;
};

#endif // CANDIDATE_MC_CRAG_CRAG_STACK_COMBINER_H__

