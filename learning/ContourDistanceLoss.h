#ifndef CANDIDATE_MC_CONTOUR_DISTANCE_LOSS_H__
#define CANDIDATE_MC_CONTOUR_DISTANCE_LOSS_H__

#include <imageprocessing/ExplicitVolume.h>
#include <learning/Loss.h>
#include <features/Diameter.h>
#include <features/HausdorffDistance.h>
#include <features/Overlap.h>

/**
 * Loss representing the difference in contour between candidate regions and the 
 * ground truth. The loss of each candidate is determined as follows:
 *
 * 1. Find the ground truth region with max overlap diameter, i.e., the ground 
 * truth region with the longest distance between two contour points of the 
 * intersection.
 *
 * 2. Get the Hausdorff distance of this ground truth region to the candidate 
 * region and vice versa.
 *
 * The loss is the sum of the two Hausdorff distances minus the overlap 
 * diameter.
 */
class ContourDistanceLoss : public Loss {

public:

	ContourDistanceLoss(
			const Crag&        crag,
			const CragVolumes& volumes,
			const Crag&        gtCrag,
			const CragVolumes& gtVolumes,
			double maxHausdorffDistance);

private:

	void getLoss(
			Crag::CragNode     n,
			const CragVolumes& volumes,
			const Crag&        gtCrag,
			const CragVolumes& gtVolumes);

	HausdorffDistance _distance;
	Overlap           _overlap;
	Diameter          _diameter;
};


#endif // CANDIDATE_MC_CONTOUR_DISTANCE_LOSS_H__

