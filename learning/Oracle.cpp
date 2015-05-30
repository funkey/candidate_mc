#include "Oracle.h"

void
Oracle::operator()(
			const std::vector<double>& weights,
			double&                    value,
			std::vector<double>&       gradient) {

	updateCosts(weights);

	MultiCut::Status status = _multicut.solve();
	_multicut.storeSolution("most-violated.tif");

	if (status != MultiCut::SolutionFound)
		UTIL_THROW_EXCEPTION(
				Exception,
				"solution not found");

	value = _constant - _multicut.getValue();

	accumulateGradient(gradient);
}

void
Oracle::updateCosts(const std::vector<double>& weights) {

	// Let E(y,w) = <w,Φy>. We have to compute the value and gradient of
	//
	//   max_y E(y',w) - E(y,w) + Δ(y',y)            (1)
	//   =
	//   max_y L(y,w)
	//
	// where y' is the best-effort solution (also known as 
	// groundtruth) and w are the current weights. The loss 
	// augmented model given by the dataset is
	//
	//   F(y,w) = E(y,w) - Δ(y',y).
	//
	// Let B_c = E(y',w) be the constant contribution of the best-effort 
	// solution. (1) is equal to
	//
	//  -min_y -B_c + F(y,w).
	//
	// Assuming that Δ(y',y) = <y,Δ_l> + Δ_c, we can rewrite F(y,w) as
	//
	//   F(y,w) = <ξ,y> - B_c - Δ_c   with   ξ = wΦ - Δ_l
	//
	// Hence, we set the costs from ξ, find the minimizer y* and the minimal 
	// value l', and set the actual value to l = B_c + Δ_c - l'.

	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
		_costs.node[n] = nodeCost(weights, _nodeFeatures[n]) - _loss.node[n];
	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e)
		_costs.edge[e] = edgeCost(weights, _edgeFeatures[e]) - _loss.edge[e];

	_constant = _loss.constant;

	// best effort energy
	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n)
		if (_bestEffort.node[n])
			_constant += nodeCost(weights, _nodeFeatures[n]);
	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e)
		if (_bestEffort.edge[e])
			_constant += edgeCost(weights, _edgeFeatures[e]);

	// L(w) = max_y <w,Φy'-Φy> + Δ(y',y)
	//      = max_y <w,Φy'-Φy> + <y,Δ_l> + Δ_c
	//      = max_y <wΦ,y'-y>  + <y,Δ_l> + Δ_c
	//      = max_y <y,-wΦ + Δ_l> + <y',wΦ> + Δ_c

	_multicut.setCosts(_costs);
}

void
Oracle::accumulateGradient(std::vector<double>& gradient) {

	// The gradient of the maximand in (1) at y* is
	//
	//   ∂L(y*,w)/∂w = ∂E(y',w)/∂w -
	//                 ∂E(y*,w)/∂w
	//
	//               = Φy' - φy*
	//               = Φ(y' - y*)
	//               = Σ_i φ_i(y'_i-y*_i)
	//                     ^^^
	//                  column vector
	//
	// which is a positive gradient contribution for the 
	// best-effort, and a negative contribution for the maximizer 
	// y*.

	std::fill(gradient.begin(), gradient.end(), 0);

	unsigned int numNodeFeatures = _nodeFeatures.dims();
	unsigned int numEdgeFeatures = _edgeFeatures.dims();

	for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

		int sign = _bestEffort.node[n] - _multicut.getSelectedRegions()[n];

		for (unsigned int i = 0; i < numNodeFeatures; i++)
			gradient[i] += _nodeFeatures[n][i]*sign;
	}

	for (Crag::EdgeIt e(_crag); e != lemon::INVALID; ++e) {

		int sign = _bestEffort.edge[e] - _multicut.getMergedEdges()[e];

		for (unsigned int i = 0; i < numEdgeFeatures; i++)
			gradient[i + numNodeFeatures] += _edgeFeatures[e][i]*sign;
	}
}
