#ifndef CANDIDATE_MC_LEARNING_BUNDLE_OPTIMIZER_HXX
#define CANDIDATE_MC_LEARNING_BUNDLE_OPTIMIZER_HXX

#include <io/CragStore.h>
#include <util/assert.h>
#include <util/helpers.hpp>
#include <util/Logger.h>
#include "learning/BundleCollector.h"
#include "solver/SolverFactory.h"

logger::LogChannel bundleoptimizerlog("bundleoptimizerlog", "[BundleOptimizer] ");

/**
 * Optimizer to optimize
 *
 *   J(w) = ½λ|w|² + L(w),
 *
 * where L(w) is provided by an oracle and can either be convex (case 1) or the 
 * difference of two convex functions (case 2):
 *
 *
 * Case 1 (Convex Optimization):
 * =============================
 *
 * Let the oracle's objective L(w) = P(w), with P(w) a convex funtion. We find
 *
 *   w* = argmin_w J(w).
 *      = argmin_w ½λ|w|² + L(w)
 *      = argmin_w ½λ|w|² + P(w)
 *
 * Case 2 (Concave-Convex Optimization):
 * =====================================
 *
 * Let the oracle's objective be the sum of a convex (P) and a concave (R) 
 * function
 *
 *   L(w) = P(w) + R(w).
 *
 * We find w* such that
 *
 *   ∂J(w)/∂w = 2λw + ∂L(w)/∂w = 0
 *
 * using the convex-concace procedure (CCCP). For that, we iteratively solve for 
 * a sequence of w*_0, w*_1, ...
 *
 *   1. ℐ_0 = inf, T = 0
 *   2. T++
 *   3. v_T = ∂R(w*_T-1)/∂w, c_T = R(w*_T-1) - <w*_T,v_T>
 *   4. w*_T = argmin_w ½λ|w|² + P(w) + <v_T,w> + c_T = argmin_w ℐ(w)
 *                                       ^^^^^    ^^^^
 *    (as for convex functions, but with this and this term added)
 *
 *      ℐ_T = min_w ℐ(w)
 *
 *   5. if ℐ_T-1 - ℐ_T ≤ η, return w*_T
 *   6. goto 2
 */
class BundleOptimizer {

public:

	enum OptimizerResult {

		// the minimal optimization gap was reached
		ReachedMinGap,

		// the requested number of steps was exceeded
		ReachedSteps,

		// something went wrong
		Error
	};

	enum EpsStrategy {

		/**
		 * Compute the eps from the gap estimate between the lower bound and the 
		 * target objective. The gap estimate will only be correct for oracle 
		 * calls that perform exact inference.
		 */
		EpsFromGap,

		/**
		 * Compute the eps from the change of the minimum of the lower bound.  
		 * This version does also work for approximate (but deterministic) 
		 * inference methods.
		 */
		EpsFromChange
	};

	struct Parameters {

		Parameters() :
			lambda(1.0),
			steps(0),
			min_eps(1e-5),
			epsStrategy(EpsFromGap),
			nu(1e-5) {}

		// regularizer weight
		double lambda;

		// the maximal number of steps to perform, 0 = no limit
		unsigned int steps;

		// bundle method stops if eps is smaller than this value
		double min_eps;

		// how to compute the eps for the stopping criterion
		EpsStrategy epsStrategy;

		// for concave-convex optimization problems, the min change in the outer 
		// loop to stop the bundle method
		double nu;
	};

	BundleOptimizer(const Parameters& parameter = Parameters());

	BundleOptimizer(std::shared_ptr<CragStore> store, const Parameters& parameter = Parameters());

	~BundleOptimizer();

	/**
	 * Start the bundle method optimization on the given oracle. The oracle has 
	 * to provide:
	 *
	 *   void operator()(
	 *			const Weights& current,
	 *			double&        value,
	 *			Weights&       gradient);
	 *
	 * and should return the value and gradient of the objective function 
	 * (passed by reference) at point 'current'. Weights has to be copy 
	 * constructable and has to provide exportToVector() and importFromVector().
	 */
	template <typename Oracle, typename Weights>
	OptimizerResult optimize(Oracle& oracle, Weights& w);

	/**
	 * Same as optimize(oracle, w), but allows to specify a binary mask on the 
	 * weights. Only non-zero entries will be updated. Use this to perform 
	 * block-coordinate descents.
	 */
	template <typename Oracle, typename Weights>
	OptimizerResult optimize(Oracle& oracle, Weights& w, Weights& mask);

	/**
	 * Get the remaining eps after optimization.
	 */
	double getEps() const { return _eps_t; }

	/**
	 * Get the minimal value after optimization.
	 *
	 * This is the smalles observed value of J(w) during the optimization.
	 */
	double getMinValue() const { return _minValue; }

private:

	/**
	 * Optimize the convex part of the oracle objective, plus the regularizer 
	 * and a linear contribution (which will be 0 for case 1, and a linear upper 
	 * bound on the concave part for case 2).
	 *
	 * Returns the optimal position in 'w'.
	 *
	 * The optimal value can be queried with getMinValue() after the 
	 * optimization finished.
	 *
	 * A binary mask can be provided to optimize only parts of the vector 
	 * (non-zero in mask).
	 */
	template <typename Oracle, typename Weights>
	OptimizerResult optimizeConvex(Oracle& oracle, Weights& w, const Weights& v_T, const Weights& mask);

    void setupQp(const std::vector<double>& w, const std::vector<double>& v_T);

	template <typename Weights>
	void findMinLowerBound(Weights& w, double& value);

	template <typename Weights>
	double dot(const Weights& a, const Weights& b);

	Parameters _parameter;

	BundleCollector _bundleCollector;

	QuadraticSolverBackend* _solver;

	double _eps_t;

	double _minValue;

	QuadraticObjective _obj;

	bool _continuePreviousQp;
};

BundleOptimizer::BundleOptimizer(const Parameters& parameter) :
	_parameter(parameter),
	_solver(0) {}

BundleOptimizer::~BundleOptimizer() {

	if (_solver)
		delete _solver;
}

template <typename Oracle, typename Weights>
BundleOptimizer::OptimizerResult
BundleOptimizer::optimize(Oracle& oracle, Weights& weights) {

	Weights mask(weights);
	mask.importFromVector(std::vector<double>(weights.exportToVector().size(), 1));
	return optimize(oracle, weights, mask);
}

template <typename Oracle, typename Weights>
BundleOptimizer::OptimizerResult
BundleOptimizer::optimize(Oracle& oracle, Weights& weights, Weights& mask) {

	_continuePreviousQp = false;

	if (!oracle.haveConcavePart()) {

		Weights v_T(weights);
		v_T.importFromVector(std::vector<double>(weights.exportToVector().size(), 0));
		return optimizeConvex(oracle, weights, v_T, mask);

	} else {

		// 1. ℐ_0 = inf, T = 0
		double J_Tm1 = std::numeric_limits<double>::infinity();
		int T = 0;

		Weights v_T = weights;

		while (true) {

			// 2. T++
			LOG_USER(bundleoptimizerlog) << std::endl << "================= CCCP iteration " << T << std::endl;
			T++;

			// 3. v_T = ∂R(w*_T-1)/∂w, c_T = R(w*_T-1) - <w*_T,v_T>
			//   i.e., linearize R at w*_T:
			double r_T;
			oracle.valueGradientR(weights, r_T, v_T);
			v_T.mask(mask);
			double c_T = r_T - dot(weights.exportToVector(), v_T.exportToVector());

			LOG_DEBUG(bundleoptimizerlog) << "   w*     = " << weights.exportToVector() << std::endl;
			LOG_DEBUG(bundleoptimizerlog) << " R(w*)    = " << r_T << std::endl;
			LOG_DEBUG(bundleoptimizerlog) << "∂R(w*)/∂w = " << v_T << std::endl;

			// 4. w*_T = argmin_w ½λ|w|² + P(w) + <v_T,w> + c_T
			//         = argmin_w ℐ_w*(w)
			if (optimizeConvex(oracle, weights, v_T, mask) != ReachedMinGap) {

				LOG_ERROR(bundleoptimizerlog) << "convex optimization did not converge" << std::endl;
				return Error;
			}

			// correct the min value (the convex optimization did not consider 
			// the constant offset c_T)
			_minValue += c_T;

			//     ℐ_T = min_w ℐ_w*(w)
			double J_T = _minValue;

			LOG_DEBUG(bundleoptimizerlog) << " min_w ℐ_w*(w) = " << weights.exportToVector() << std::endl;
			LOG_DEBUG(bundleoptimizerlog) << " min   ℐ_w*(w) = " << J_T << std::endl;
			LOG_DEBUG(bundleoptimizerlog) << "   η           = " << (J_Tm1 - J_T) << std::endl;

			// 5. if ℐ_T-1 - ℐ_T ≤ η, return w*_T
			if (J_Tm1 - J_T <= _parameter.nu)
				return ReachedMinGap;

			J_Tm1 = J_T;

			// after the first iteration, the QP to solve min P(w) should not be 
			// rebuild
			_continuePreviousQp = true;
		}
	}
}

template <typename Oracle, typename Weights>
BundleOptimizer::OptimizerResult
BundleOptimizer::optimizeConvex(Oracle& oracle, Weights& weights, const Weights& v_T, const Weights& mask) {

	LOG_ALL(bundleoptimizerlog)
			<< "starting convex optimization using eps from "
			<< (_parameter.epsStrategy == EpsFromChange ? "change" : "gap")
			<< " strategy" << std::endl;

	/*
	 * Here, we minimize either:
	 *
	 * Case 1 (Convex Optimization):
	 * =============================
	 *
	 *   A convex function L(w) with a quadratic regularizer:
	 *
	 *   ½λ|w|² + L(w) =
	 *   ½λ|w|² + P(w)
	 *
	 * Case 2 (Concave-Convex Optimization):
	 * =====================================
	 *
	 *   An approximation of L(w) = P(w) + R(w) with R(w) linearized
	 *
	 *   ½λ|w|² + P(w) + <v_T,w>
	 *
	 * The optimization problem is the same in either case, with v_T = 0 in case 
	 * 1.
	 */

	Weights gradient(weights);

	std::vector<double> w = weights.exportToVector();

	setupQp(w, v_T.exportToVector());

	/*
	  1. w_0 = 0, t = 0
	  2. t++
	  3. compute a_t = ∂P(w_t-1)/∂w
	  4. compute b_t =  P(w_t-1) - <w_t-1,a_t>
	  5. ℘_t(w) = max_i <w,a_i> + b_i
	  6. w_t = argmin λ½|w|² + ℘_t(w) + <w,v_T>
	  7. ε_t = min_i [ λ½|w_i|² + P(w_i) + <w,v_T> ] - [ λ½|w_t|² + ℘_t(w_t) + <w,v_T> ]
			   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^^^^^^^^^
				 smallest J(w) ever seen               current min of lower bound
	  8. if ε_t > ε, goto 2
	  9. return w_t
	*/

	_minValue =  std::numeric_limits<double>::infinity();
	double lastMinLower = -std::numeric_limits<double>::infinity();

	unsigned int t = 0;

	while (_parameter.steps == 0 || t < _parameter.steps) {

		LOG_USER(bundleoptimizerlog) << std::endl << "----------------- iteration " << t << std::endl;

		t++;

		LOG_ALL(bundleoptimizerlog) << "current w is " << w << std::endl;

		// value of P at current w
		double P_w_tm1 = 0.0;

		// gradient of P at current w
        std::vector<double> a_t(w.size());

		// get current value and gradient of P(w)
		oracle.valueGradientP(weights, P_w_tm1, gradient);
		gradient.mask(mask);
		a_t = gradient.exportToVector();

		LOG_DEBUG(bundleoptimizerlog) << "       P(w)              is: " << P_w_tm1 << std::endl;
		LOG_ALL(bundleoptimizerlog)   << "      ∂P(w)/∂            is: " << a_t << std::endl;

		// update smallest observed value of ½λ|w|² + P(w) + <w,v_T>
		_minValue = std::min(_minValue, _parameter.lambda*0.5*dot(w, w) + P_w_tm1 + dot(w, v_T.exportToVector()));

		LOG_DEBUG(bundleoptimizerlog) << " min_i ½λ|w_i|² + P(w_i) + <w_i,v_T> is: " << _minValue << std::endl;

		// compute hyperplane offset
		double b_t = P_w_tm1 - dot(w, a_t);

		//LOG_ALL(bundleoptimizerlog) << "adding hyperplane " << a_t << "*w + " << b_t << std::endl;

		// update lower bound
		_bundleCollector.addHyperplane(a_t, b_t);

		// minimal value of lower bound
		double minLower;

		// update w and get minimal value
		findMinLowerBound(w, minLower);

		// update weights data structure
		weights.importFromVector(w);

		LOG_DEBUG(bundleoptimizerlog) << " min_w ℘(w)   + ½λ|w|²   is: " << minLower << std::endl;
		//LOG_ALL(bundleoptimizerlog) << " w* of ℘(w)   + ½λ|w|²   is: "  << w 
		//<< std::endl;

		// compute gap
		if (_parameter.epsStrategy == EpsFromGap)
			_eps_t = _minValue - minLower;
		else
			_eps_t = minLower - lastMinLower;

		lastMinLower = minLower;

		LOG_USER(bundleoptimizerlog)  << "          ε   is: " << _eps_t << std::endl;

		// converged?
		if (_parameter.min_eps > 0 && _eps_t <= _parameter.min_eps)
			break;
	}

	if (t == _parameter.steps)
			return ReachedSteps;

	return ReachedMinGap;
}

void
BundleOptimizer::setupQp(const std::vector<double>& w, const std::vector<double>& v_T) {

	/*
	  w* = argmin λ½|w|² + <v_T,w> + ξ, s.t. <w,a_i> + b_i ≤ ξ ∀i
	*/

	if (!_solver) {

		SolverFactory factory;
		_solver = factory.createQuadraticSolverBackend();
	}

	if (!_continuePreviousQp) {

		_solver->initialize(w.size() + 1, Continuous);

		// one variable for each component of w and for ξ
		_obj.resize(w.size() + 1);

		// regularizer
		for (unsigned int i = 0; i < w.size(); i++)
			_obj.setQuadraticCoefficient(i, i, 0.5*_parameter.lambda);

		// ξ
		_obj.setCoefficient(w.size(), 1.0);

		// we minimize
		_obj.setSense(Minimize);
	}

	// <v_T,w>
    for (unsigned int i = 0; i < w.size(); i++)
		_obj.setCoefficient(i, v_T[i]);

	// we are done with the objective -- this does not change anymore
	_solver->setObjective(_obj);
}

template <typename Weights>
void
BundleOptimizer::findMinLowerBound(Weights& w, double& value) {

	for (const LinearConstraint& constraint : _bundleCollector.getNewConstraints())
		_solver->addConstraint(constraint);

	Solution x;
	std::string msg;
	bool optimal = _solver->solve(x, msg);
	value = x.getValue();

	if (!optimal) {

		std::cerr
				<< "[BundleOptimizer] QP could not be solved to optimality: "
				<< msg << std::endl;

		return;
	}

	for (size_t i = 0; i < w.size(); i++)
		w[i] = x[i];
}

template <typename Weights>
double
BundleOptimizer::dot(const Weights& a, const Weights& b) {

	UTIL_ASSERT_REL(a.size(), ==, b.size());

	double d = 0.0;
	for (size_t i = 0; i < a.size(); i++)
		d += a[i]*b[i];

	return d;
}

#endif // CANDIDATE_MC_LEARNING_BUNDLE_OPTIMIZER_HXX

