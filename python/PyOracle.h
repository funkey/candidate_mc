#ifndef CANDIDATE_MC_PYTHON_PY_ORACLE_H__
#define CANDIDATE_MC_PYTHON_PY_ORACLE_H__

#include <vector>
#include <boost/python.hpp>

class PyOracle {

public:

	/**
	 * Simple wrapper around double to pass it by reference to the python oracle.
	 */
	struct Value {

		double v;
	};

	/**
	 * Simple wrapper around std::vector to use it as weights in the 
	 * BundleOptimizer.
	 */
	class Weights : public std::vector<double> {

	public:

		Weights(std::size_t s) : std::vector<double>(s, 0) {}

		void importFromVector(const std::vector<double>& w) {

			static_cast<std::vector<double>&>(*this) = w;
		}

		const std::vector<double>& exportToVector() {

			return *this;
		}
	};

	/**
	 * Set a python function object to be called for evaluating the current 
	 * value and gradient at w:
	 *
	 *   pyCallback(w, value, gradient)
	 */
	void setEvaluateFunctor(boost::python::api::object pyCallback) {

		_pyCallback = pyCallback;
	}

	void operator()(
			const Weights& weights,
			double&        value,
			Weights&       gradient);

private:

	boost::python::api::object _pyCallback;
};

#endif // CANDIDATE_MC_PYTHON_PY_ORACLE_H__

