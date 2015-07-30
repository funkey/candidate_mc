#ifndef CANDIDATE_MC_LEARNING_GRADIENT_OPTIMIZER_H__
#define CANDIDATE_MC_LEARNING_GRADIENT_OPTIMIZER_H__

#include <io/vectors.h>
#include <util/assert.h>
#include <util/helpers.hpp>
#include <util/Logger.h>

logger::LogChannel gradientoptimizerlog("gradientoptimizerlog", "[GradientOptimizer] ");

/**
 * Optimizer to optimize an Oracle's objective plus a quadratic regularizer 
 * ½λ|w|².
 */
class GradientOptimizer {

public:

	enum OptimizerResult {

		// the minimal gradient magnitude was reached
		ReachedMinGradient,

		// the requested number of steps was exceeded
		ReachedSteps,

		// something went wrong
		Error
	};

	struct Parameters {

		Parameters() :
			lambda(1.0),
			initialStepWidth(1.0),
			stepWithDecrease(0.99),
			steps(0),
			minGradientMagnitude(1e-5) {}

		// regularizer weight
		double lambda;

		double initialStepWidth;

		// a factor to decrease the step width with each iteration
		double stepWithDecrease;

		// the maximal number of steps to perform, 0 = no limit
		unsigned int steps;

		// gradient method stops if the gradient magnitude is smaller than this 
		// value
		double minGradientMagnitude;
	};

	GradientOptimizer(const Parameters& parameter = Parameters());

	/**
	 * Start the gradient method optimization on the given oracle. The oracle has 
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
	double dot(const Weights& a, const Weights& b);

	Parameters _parameter;
};

GradientOptimizer::GradientOptimizer(const Parameters& parameter) :
	_parameter(parameter) {}

template <typename Oracle, typename Weights>
GradientOptimizer::OptimizerResult
GradientOptimizer::optimize(Oracle& oracle, Weights& w) {

	unsigned int t = 0;

	while (true) {

		LOG_USER(gradientoptimizerlog) << std::endl << "----------------- iteration " << t << std::endl;

		t++;

		LOG_ALL(gradientoptimizerlog) << "current w is " << w << std::endl;

		// value of L at current w
		double value = 0.0;

		// gradient of L at current w
        Weights gradient(w.size());

		// get current value and gradient
		oracle(w, value, gradient);

		LOG_DEBUG(gradientoptimizerlog) << "       L(w)              is: " << value << std::endl;
		LOG_ALL(gradientoptimizerlog)   << "      ∂L(w)/∂            is: " << gradient << std::endl;

		double stepWidth = _parameter.initialStepWidth/(t + 1);

		// ∂L(w)/∂ + ∂λ½|w|²/∂ = ∂L(w)/∂ + λw
		for (int i = 0; i < w.size(); i++)
			w[i] -= stepWidth*gradient[i] + _parameter.lambda*w[i];

		storeVector(w, std::string("feature_weights_") + boost::lexical_cast<std::string>(t) + ".txt");

		double magnitude2 = 0;
		for (double v : gradient)
			magnitude2 += v*v;

		LOG_DEBUG(gradientoptimizerlog) << "     |∂L(w)/∂|           is: " << sqrt(magnitude2) << std::endl;
		LOG_DEBUG(gradientoptimizerlog) << "     step width          is: " << stepWidth << std::endl;

		// converged?
		if (magnitude2 <= _parameter.minGradientMagnitude*_parameter.minGradientMagnitude)
			break;

		if (_parameter.steps > 0 && t >= _parameter.steps)
			return ReachedSteps;
	}

	return ReachedMinGradient;
}

template <typename Weights>
double
GradientOptimizer::dot(const Weights& a, const Weights& b) {

	UTIL_ASSERT_REL(a.size(), ==, b.size());

	double d = 0.0;
	for (size_t i = 0; i < a.size(); i++)
		d += a[i]*b[i];

	return d;
}


#endif // CANDIDATE_MC_LEARNING_GRADIENT_OPTIMIZER_H__

