#ifndef CANDIDATE_MC_FEATURES_SKELETON_EXTRACTOR_H__
#define CANDIDATE_MC_FEATURES_SKELETON_EXTRACTOR_H__

#include <io/CragStore.h>
#include <imageprocessing/ExplicitVolume.h>
#include <imageprocessing/Skeletonize.h>
#include "Skeletons.h"

class SkeletonExtractor {

public:

	SkeletonExtractor(const Crag& crag) :
		_crag(crag) {}

	/**
	 * Extract the skeletons for all tubes in the given store, and store them in 
	 * the same store.
	 */
	void extract(Skeletons& skeletons);

private:

	ExplicitVolume<float> downsampleVolume(const ExplicitVolume<unsigned char>& volume);

	const Crag&  _crag;
};

#endif // CANDIDATE_MC_FEATURES_SKELETON_EXTRACTOR_H__

