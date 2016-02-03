#include "PyOracle.h"

void
PyOracle::operator()(
		const PyOracle::Weights& weights,
		double&                  value,
		PyOracle::Weights&       gradient) {

	// forward the oracle call to the python callback
	_pyCallback(weights, value, gradient);
}
