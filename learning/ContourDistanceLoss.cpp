#include <io/CragImport.h>
#include <util/Logger.h>
#include <util/timing.h>
#include <imageprocessing/intersect.h>
#include "ContourDistanceLoss.h"

logger::LogChannel contourdistancelosslog("contourdistancelosslog", "[ContourDistanceLoss] ");

ContourDistanceLoss::ContourDistanceLoss(
		const Crag&        crag,
		const CragVolumes& volumes,
		const Crag&        gtCrag,
		const CragVolumes& gtVolumes,
		double maxHausdorffDistance) :
	Loss(crag),
	_distance(maxHausdorffDistance) {

	UTIL_TIME_METHOD;

	// loss for each candidate
	for (Crag::CragNode n : crag.nodes()) {

		getLoss(n, volumes, gtCrag, gtVolumes);
		LOG_ALL(contourdistancelosslog)
				<< "loss of node " << crag.id(n)
				<< " at " << volumes[n]->getBoundingBox()
				<< ": " << node[n] << std::endl;
	}

	_distance.clearCache();

	LOG_USER(contourdistancelosslog) << "done." << std::endl;
}

void
ContourDistanceLoss::getLoss(
		Crag::CragNode     n,
		const CragVolumes& volumes,
		const Crag&        gtCrag,
		const CragVolumes& gtVolumes) {

	std::shared_ptr<CragVolume> volume = volumes[n];

	double maxOverlapDiameter = 0;
	std::shared_ptr<CragVolume> bestGtRegion;

	for (Crag::CragNode gt : gtCrag.nodes()) {

		std::shared_ptr<CragVolume> gtVolume = gtVolumes[gt];

		// does overlap?
		if (!_overlap.exceeds(*volume, *gtVolume, 0))
			continue;

		// reward

		CragVolume overlap;
		intersect(*volume, *gtVolume, overlap);

		double overlapDiameter = _diameter(overlap);

		if (maxOverlapDiameter < overlapDiameter) {

			maxOverlapDiameter = overlapDiameter;
			bestGtRegion = gtVolume;
		}
	}

	// penalty

	double penalty = 0;

	if (bestGtRegion) {

		LOG_ALL(contourdistancelosslog)
				<< "best gt region is at "
				<< bestGtRegion->getBoundingBox()
				<< std::endl;

		double gtToCandidate, candidateToGt;
		_distance(
				*bestGtRegion,
				*volume,
				gtToCandidate,
				candidateToGt);

		LOG_ALL(contourdistancelosslog)
				<< "distance to candidate: " << gtToCandidate
				<< ", distance to gt: " << candidateToGt
				<< std::endl;

		penalty += gtToCandidate + candidateToGt;

	} else {

		LOG_ALL(contourdistancelosslog)
				<< "no overlapping gt region found"
				<< std::endl;
	}

	// set the loss
	node[n] = penalty - maxOverlapDiameter;

	// add the constant (the maximally possible overlap with any ground truth 
	// region, i.e., the diameter of the candidate)
	double diameter = _diameter(*volume);
	constant += diameter;
}
