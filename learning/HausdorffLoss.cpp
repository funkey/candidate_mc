#include <io/CragImport.h>
#include <features/Diameter.h>
#include "HausdorffLoss.h"

HausdorffLoss::HausdorffLoss(
		const Crag&        crag,
		const CragVolumes& volumes,
		const Crag&        gtCrag,
		const CragVolumes& gtVolumes,
		double maxHausdorffDistance) :
	Loss(crag) {

	Diameter          diameter;
	HausdorffDistance hausdorff(maxHausdorffDistance);

	for (Crag::CragNode n : crag.nodes()) {

		double loss = diameter(*volumes[n]);

		for (Crag::CragNode gt : gtCrag.nodes()) {

			double i_j, j_i;
			hausdorff(*volumes[n], *gtVolumes[gt], i_j, j_i);

			loss = std::min(loss, std::max(i_j, j_i));
		}

		node[n] = loss;
	}
}
