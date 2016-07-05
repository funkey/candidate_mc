#ifndef CANDIDATE_MC_PYTHON_PY_ORACLE_H__
#define CANDIDATE_MC_PYTHON_PY_ORACLE_H__

#include <algorithm>
#include <vector>
#include <boost/python.hpp>
#include <learning/Oracle.h>
#include <util/assert.h>

/**
 * Simple wrapper around std::vector to use it as weights in the 
 * BundleOptimizer.
 */
class PyOracleWeights : public std::vector<double> {

public:

	PyOracleWeights(std::size_t s) : std::vector<double>(s, 0) {}

	void importFromVector(const std::vector<double>& w) {

		static_cast<std::vector<double>&>(*this) = w;
	}

	const std::vector<double>& exportToVector() const {

		return *this;
	}

	/**
	 * Set all weights that are zero in mask to zero.
	 */
	void mask(const PyOracleWeights& mask) {

		UTIL_ASSERT_REL(size(), ==, mask.size());

		auto mask_op = [](double x, double m) { return (m == 0 ? 0 : x ); };

		std::transform(
				begin(),
				end(),
				mask.begin(),
				begin(),
				mask_op);
	}
};

/**
 * An oracle to be used in a generic optimizer. The oracle is assumed to 
 * represent
 *
 *   L(w) = P(w) - R(w),
 *
 * where P and R are convex. If no callback is registered for R, it will not be 
 * considered by the optimizers, resulting in standard convex optimization.
 */
class PyOracle : public Oracle<PyOracleWeights> {

public:

	/**
	 * Simple wrapper around double to pass it by reference to the python oracle.
	 */
	struct Value {

		double v;
	};

	PyOracle() : _haveConcavePart(false) {}

	/**
	 * Set a python function object to be called for evaluating the current 
	 * value and gradient at w:
	 *
	 *   pyCallback(w, value, gradient)
	 */
	void setValueGradientPCallback(boost::python::api::object pyCallback) {

		_callbackP = pyCallback;
	}

	/**
	 * Set a python function object to be called for evaluating the current 
	 * value and gradient at w:
	 *
	 *   pyCallback(w, value, gradient)
	 */
	void setValueGradientRCallback(boost::python::api::object pyCallback) {

		_haveConcavePart = true;
		_callbackR = pyCallback;
	}

	void valueGradientP(
			const PyOracleWeights& weights,
			double&                value,
			PyOracleWeights&       gradient) override;

	void valueGradientR(
			const PyOracleWeights& weights,
			double&                value,
			PyOracleWeights&       gradient) override;

	bool haveConcavePart() const { return _haveConcavePart; }

private:

	boost::python::api::object _callbackP;
	boost::python::api::object _callbackR;

	bool _haveConcavePart;
};

#endif // CANDIDATE_MC_PYTHON_PY_ORACLE_H__

