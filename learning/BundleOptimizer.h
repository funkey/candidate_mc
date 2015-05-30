#ifndef CANDIDATE_MC_LEARNING_BUNDLE_OPTIMIZER_HXX
#define CANDIDATE_MC_LEARNING_BUNDLE_OPTIMIZER_HXX

#include <util/assert.h>
#include <util/helpers.hpp>
#include "learning/BundleCollector.h"
#include "solver/DefaultFactory.h"

enum OptimizerResult {

	// the minimal optimization gap was reached
	ReachedMinGap,

	// the requested number of steps was exceeded
	ReachedSteps,

	// something went wrong
	Error
};

class BundleOptimizer {

public:

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
			epsStrategy(EpsFromChange) {}

		// regularizer weight
		double lambda;

		// the maximal number of steps to perform, 0 = no limit
		unsigned int steps;

		// bundle method stops if eps is smaller than this value
		double min_eps;

		// how to compute the eps for the stopping criterion
		EpsStrategy epsStrategy;
	};

	BundleOptimizer(const Parameters& parameter = Parameters());

	~BundleOptimizer();

	/**
	 * Start the bundle method optimization on the given oracle. The oracle has 
	 * to provide:
	 *
	 *   void operator()(
	 *			const std::vector<double>& current,
	 *			double&                    value,
	 *			std::vector<double>&       gradient);
	 *
	 * and should return the value and gradient of the objective function 
	 * (passed by reference) at point 'current'.
	 */
    template <typename Oracle, typename Weights>
    OptimizerResult optimize(Oracle& oracle, Weights& w);

private:

    template <typename Weights>
    void setupQp(const Weights& w);

	template <typename Weights>
	void findMinLowerBound(Weights& w, double& value);

	template <typename Weights>
	double dot(const Weights& a, const Weights& b);

	Parameters _parameter;

	BundleCollector _bundleCollector;

	QuadraticSolverBackend* _solver;
};

BundleOptimizer::BundleOptimizer(const Parameters& parameter) :
	_parameter(parameter),
	_solver(0) {}

BundleOptimizer::~BundleOptimizer() {

	if (_solver)
		delete _solver;
}

template <typename Oracle, typename Weights>
OptimizerResult
BundleOptimizer::optimize(Oracle& oracle, Weights& w) {

	setupQp(w);

	/*
	  1. w_0 = 0, t = 0
	  2. t++
	  3. compute a_t = ∂L(w_t-1)/∂w
	  4. compute b_t =  L(w_t-1) - <w_t-1,a_t>
	  5. ℒ_t(w) = max_i <w,a_i> + b_i
	  6. w_t = argmin λ½|w|² + ℒ_t(w)
	  7. ε_t = min_i [ λ½|w_i|² + L(w_i) ] - [ λ½|w_t|² + ℒ_t(w_t) ]
			   ^^^^^^^^^^^^^^^^^^^^^^^^^^^   ^^^^^^^^^^^^^^^^^^^^^^^
				 smallest L(w) ever seen    current min of lower bound
	  8. if ε_t > ε, goto 2
	  9. return w_t
	*/

	double minValue     =  std::numeric_limits<double>::infinity();
	double lastMinLower = -std::numeric_limits<double>::infinity();

	unsigned int t = 0;

	while (true) {

		t++;

		std::cout << std::endl << "----------------- iteration " << t << std::endl;

        Weights w_tm1 = w;

		std::cout << "current w is " << w_tm1 << std::endl;

		// value of L at current w
		double L_w_tm1 = 0.0;

		// gradient of L at current w
        Weights a_t(w.size());

		// get current value and gradient
		oracle(w_tm1, L_w_tm1, a_t);

		std::cout << "       L(w)              is: " << L_w_tm1 << std::endl;
		std::cout << "      ∂L(w)/∂            is: " << a_t << std::endl;

		// update smallest observed value of regularized L
		minValue = std::min(minValue, L_w_tm1 + _parameter.lambda*0.5*dot(w_tm1, w_tm1));

		std::cout << " min_i L(w_i) + ½λ|w_i|² is: " << minValue << std::endl;

		// compute hyperplane offset
		double b_t = L_w_tm1 - dot(w_tm1, a_t);

		//std::cout << "adding hyperplane " << a_t << "*w + " << b_t << std::endl;

		// update lower bound
		_bundleCollector.addHyperplane(a_t, b_t);

		// minimal value of lower bound
		double minLower;

		// update w and get minimal value
		findMinLowerBound(w, minLower);

		std::cout << " min_w ℒ(w)   + ½λ|w|²   is: " << minLower << std::endl;
		//std::cout << " w* of ℒ(w)   + ½λ|w|²   is: "  << w << std::endl;

		// compute gap
		double eps_t;
		if (_parameter.epsStrategy == EpsFromGap)
			eps_t = minValue - minLower;
		else
			eps_t = minLower - lastMinLower;

		lastMinLower = minLower;

		std::cout  << "          ε   is: " << eps_t << std::endl;

		// converged?
		if (eps_t <= _parameter.min_eps)
			break;
	}

	return ReachedMinGap;
}

template <typename Weights>
void
BundleOptimizer::setupQp(const Weights& w) {

	/*
	  w* = argmin λ½|w|² + ξ, s.t. <w,a_i> + b_i ≤ ξ ∀i
	*/

	if (!_solver) {

		DefaultFactory factory;
		_solver = factory.createQuadraticSolverBackend();
	}

	_solver->initialize(w.size() + 1, Continuous);

	// one variable for each component of w and for ξ
    QuadraticObjective obj(w.size() + 1);

	// regularizer
    for (unsigned int i = 0; i < w.size(); i++)
		obj.setQuadraticCoefficient(i, i, 0.5*_parameter.lambda);

	// ξ
    obj.setCoefficient(w.size(), 1.0);

	// we minimize
	obj.setSense(Minimize);

	// we are done with the objective -- this does not change anymore
	_solver->setObjective(obj);
}

template <typename Weights>
void
BundleOptimizer::findMinLowerBound(Weights& w, double& value) {

	_solver->setConstraints(_bundleCollector.getConstraints());

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
