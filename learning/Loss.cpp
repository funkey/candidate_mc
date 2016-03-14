#include "Loss.h"
#include <util/Logger.h>

logger::LogChannel losslog("losslog", "[Loss] ");

void
Loss::normalize(const Crag& crag, const MultiCutSolver::Parameters& params) {

	double min, max;

	{
		LOG_DEBUG(losslog) << "searching for minimal loss value..." << std::endl;

		MultiCutSolver::Parameters minParams(params);
		CragSolution solution(crag);
		minParams.minimize = true;
		MultiCutSolver multicut(crag, minParams);
		multicut.setCosts(*this);
		multicut.solve(solution);
		min = multicut.getValue();

		LOG_DEBUG(losslog) << "minimal value is " << min << std::endl;
	}
	{
		LOG_DEBUG(losslog) << "searching for maximal loss value..." << std::endl;

		MultiCutSolver::Parameters maxParams(params);
		CragSolution solution(crag);
		maxParams.minimize = false;
		MultiCutSolver multicut(crag, maxParams);
		multicut.setCosts(*this);
		multicut.solve(solution);
		max = multicut.getValue();

		LOG_DEBUG(losslog) << "maximal value is " << max << std::endl;
	}

	// All energies are between E(y^min) and E(y^max). We want E(y^min) to be 
	// zero, and E(y^max) to be 1. Therefore, we subtract min from each E(y) and 
	// scale it with 1.0/(max - min):
	//
	//  (E(y) + offset)*scale
	//
	//           = ([sum_i E(y_i)] + offset)*scale
	//           = ([sum_i E(y_i)]*scale + offset*scale)
	//
	// -> We scale each loss by scale, and add offset*scale to the constant.

	double offset = -min;
	double scale  = 1.0/(max - min);

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		node[n] *= scale;
	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e)
		edge[e] *= scale;

	constant += offset*scale;
}

