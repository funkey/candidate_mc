#ifndef CANDIDATE_MC_LEARNING_HAUSDORFF_LOSS_H__
#define CANDIDATE_MC_LEARNING_HAUSDORFF_LOSS_H__

#include <imageprocessing/ExplicitVolume.h>
#include <learning/Loss.h>
#include <features/HausdorffDistance.h>

/**
 * Loss that reflects the minimal hausdorff distance of each region with the 
 * ground truth.
 */
class HausdorffLoss : public Loss {

public:

	HausdorffLoss(
			const Crag&                crag,
			const CragVolumes&         volumes,
			const ExplicitVolume<int>& groundTruth,
			double maxHausdorffDistance);
};

#endif // CANDIDATE_MC_LEARNING_HAUSDORFF_LOSS_H__

