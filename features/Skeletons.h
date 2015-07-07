#ifndef CANDIDATE_MC_FEATURES_SKELETONS_H__
#define CANDIDATE_MC_FEATURES_SKELETONS_H__

#include <imageprocessing/Skeleton.h>
#include "Crag.h"

class Skeletons : public Crag::NodeMap<Skeleton> {

public:

	/**
	 * Create skeleton map for the given CRAG.
	 */
	Skeletons(Crag& crag) :
			Crag::NodeMap<Skeleton>(crag) {}
};

#endif // CANDIDATE_MC_FEATURES_SKELETONS_H__

