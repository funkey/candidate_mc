#include "Oracle.h"
#include <sstream>
#include <util/ProgramOptions.h>
#include <util/Logger.h>

logger::LogChannel oraclelog("oraclelog", "[Oracle] ");

util::ProgramOption optionStoreEachMostViolated(
		util::_long_name        = "storeMostViolated",
		util::_description_text = "In each training iteration, store the currently most violated solution.");

util::ProgramOption optionStoreEachCurrentlyBest(
		util::_long_name        = "storeCurrentlyBest",
		util::_description_text = "In each training iteration, store the currently best solution.");

void
Oracle::operator()(
			const FeatureWeights& weights,
			double&               value,
			FeatureWeights&       gradient) {

	updateCosts(weights);

	CragSolver::Status status = _mostViolatedSolver->solve();

	//if (optionStoreEachMostViolated) {

		//std::stringstream filename;
		//filename << "most-violated_" << std::setw(6) << std::setfill('0') << _iteration << ".tif";
		//_mostViolatedSolver->storeSolution(_volumes, filename.str(), true);
	//}

	if (status != CragSolver::SolutionFound)
		UTIL_THROW_EXCEPTION(
				Exception,
				"solution not found");

	value = _constant - _mostViolatedSolver->getValue();

	// value = E(y',w) - E(y*,w) + Δ(y',y*)
	//       = B_c - <wΦ,y*> + <Δ_l,y*> + Δ_c

	// loss   = value - B_c + <wΦ,y*>
	// margin = value - loss

	double mostViolatedEnergy = 0;
	for (Crag::CragNode n : _crag.nodes())
		if (_mostViolatedSolver->getSolution().selected(n))
			mostViolatedEnergy += nodeCost(n, weights);
	for (Crag::CragEdge e : _crag.edges())
		if (_mostViolatedSolver->getSolution().selected(e))
			mostViolatedEnergy += edgeCost(e, weights);

	double loss   = value - _B_c + mostViolatedEnergy;
	double margin = value - loss;

	LOG_USER(oraclelog) << "Δ(y*)         = " << loss << std::endl;
	LOG_USER(oraclelog) << "E(y') - E(y*) = " << margin << std::endl;

	accumulateGradient(gradient);

	//if (optionStoreEachCurrentlyBest) {

		//_currentBestSolver->solve();

		//std::stringstream filename;
		//filename << "current-best_" << std::setw(6) << std::setfill('0') << _iteration << ".tif";
		//_currentBestSolver->storeSolution(_volumes, filename.str(), true);
	//}

	_iteration++;
}

void
Oracle::updateCosts(const FeatureWeights& weights) {

	// Let E(y,w) = <w,Φy>. We have to compute the value and gradient of
	//
	//   max_y L(y,w)
	//   =
	//   max_y E(y',w) - E(y,w) + Δ(y',y)            (1)
	//
	// where y' is the best-effort solution (also known as groundtruth) and w 
	// are the current weights. The loss augmented model to solve is
	//
	//   F(y,w) = E(y,w) - Δ(y',y).
	//
	// Let B_c = E(y',w) be the constant contribution of the best-effort 
	// solution. (1) is equal to
	//
	//   max_y  B_c -  E(y,w) + Δ(y',y)
	//   =
	//   max_y  B_c - (E(y,w) - Δ(y',y))
	//   =
	//   max_y  B_c - F(y,w)
	//   =
	//  -min_y -B_c + F(y,w)
	//   =
	//  -(-B_c + min_y F(y,w))
	//   =
	//   B_c - min_y F(y,w).                         (1')
	//
	// Assuming that Δ(y',y) = <y,Δ_l> + Δ_c, we can rewrite F(y,w) as
	//
	//   F(y,w) = <wΦ,y> - <Δ_l,y> - Δ_c
	//          = <ξ,y>  - Δ_c           with   ξ = wΦ - Δ_l
	//          = <ξ,y>  + c             with   c = -Δ_c
	//
	// Hence, we set the multicut costs to ξ, find the minimizer y* and the 
	// minimal value v*. y* is the minimizer of (1') and therefore also of (1).
	//
	// v* is the minimal value of F(y,w) - c. Hence, v* + c is the minimal value 
	// of F(y,w), and B_c - (v* + c) is the value of (1'), and thus the value l* 
	// of (1):
	//
	//   l* = B_c - (v* + c)
	//      = B_c - (v* - Δ_c)
	//      = B_c + Δ_c - v*.
	//
	// We store B_c + Δ_c in _constant, and subtract v* from it to get the 
	// value.

	// wΦ
	for (Crag::CragNode n : _crag.nodes())
		_costs.node[n] = nodeCost(n, weights);
	for (Crag::CragEdge e : _crag.edges())
		_costs.edge[e] = edgeCost(e, weights);

	_currentBestSolver->setCosts(_costs);

	// -Δ_l
	for (Crag::CragNode n : _crag.nodes())
		_costs.node[n] -= _loss.node[n];
	for (Crag::CragEdge e : _crag.edges())
		_costs.edge[e] -= _loss.edge[e];

	// Δ_c
	_constant = _loss.constant;

	// B_c
	_B_c = 0;
	for (Crag::CragNode n : _crag.nodes())
		if (_bestEffort.node[n])
			_B_c += nodeCost(n, weights);
	for (Crag::CragEdge e : _crag.edges())
		if (_bestEffort.edge[e])
			_B_c += edgeCost(e, weights);
	_constant += _B_c;

	// L(w) = max_y <w,Φy'-Φy> + Δ(y',y)
	//      = max_y <w,Φy'-Φy> + <y,Δ_l> + Δ_c
	//      = max_y <wΦ,y'-y>  + <y,Δ_l> + Δ_c
	//      = max_y <y,-wΦ + Δ_l> + <y',wΦ> + Δ_c

	_mostViolatedSolver->setCosts(_costs);
}

void
Oracle::accumulateGradient(FeatureWeights& gradient) {

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

	gradient.fill(0);

	for (Crag::CragNode n : _crag.nodes()) {

		int sign = _bestEffort.node[n] - _mostViolatedSolver->getSolution().selected(n);

		const std::vector<double>& f = _nodeFeatures[n];
		std::vector<double>&       g = gradient[_crag.type(n)];
		for (unsigned int i = 0; i < f.size(); i++)
			g[i] += f[i]*sign;
	}

	for (Crag::CragEdge e : _crag.edges()) {

		int sign = _bestEffort.edge[e] - _mostViolatedSolver->getSolution().selected(e);

		const std::vector<double>& f = _edgeFeatures[e];
		std::vector<double>&       g = gradient[_crag.type(e)];
		for (unsigned int i = 0; i < f.size(); i++)
			g[i] += f[i]*sign;
	}
}
