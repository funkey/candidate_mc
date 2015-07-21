#ifndef CANDIDATE_MC_FEATURES_SKELETON_EXTRACTOR_H__
#define CANDIDATE_MC_FEATURES_SKELETON_EXTRACTOR_H__

#include <imageprocessing/ExplicitVolume.h>
#include <imageprocessing/Skeletonize.h>
#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include "Skeletons.h"

class SkeletonExtractor {

public:

	SkeletonExtractor(const Crag& crag, const CragVolumes& volumes) :
		_crag(crag),
		_volumes(volumes) {}

	/**
	 * Extract the skeletons for all candidates in the given CRAG.
	 *
	 * @param[out] skeletons
	 *                   An empty node map to store the skeletons for each node 
	 *                   in the CRAG.
	 */
	void extract(Skeletons& skeletons);

private:

	ExplicitVolume<float> downsampleVolume(const CragVolume& volume);

	const Crag&        _crag;
	const CragVolumes& _volumes;
};

#endif // CANDIDATE_MC_FEATURES_SKELETON_EXTRACTOR_H__

