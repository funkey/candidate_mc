#include <io/CragImport.h>
#include <features/Diameter.h>
#include "HausdorffLoss.h"

HausdorffLoss::HausdorffLoss(
		const Crag&                crag,
		const CragVolumes&         volumes,
		const ExplicitVolume<int>& groundTruth,
		double maxHausdorffDistance) :
	Loss(crag) {

	Crag        gtCrag;
	CragVolumes gtVolumes(gtCrag);

	CragImport import;
	import.readSupervoxels(groundTruth, gtCrag, gtVolumes, groundTruth.getResolution(), groundTruth.getOffset());

	Diameter          diameter;
	HausdorffDistance hausdorff(volumes, gtVolumes, maxHausdorffDistance);

	for (Crag::CragNode n : crag.nodes()) {

		double loss = diameter(*volumes[n]);

		for (Crag::CragNode gt : gtCrag.nodes()) {

			double i_j, j_i;
			hausdorff(n, gt, i_j, j_i);

			loss = std::min(loss, std::max(i_j, j_i));
		}

		node[n] = loss;
	}
}
