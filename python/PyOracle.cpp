#include "PyOracle.h"

void
PyOracle::operator()(
		const PyOracle::Weights& weights,
		double&                  value,
		PyOracle::Weights&       gradient) {

	// forward the oracle call to the python callback
	Value v;
	_pyCallback(weights, boost::ref(v), boost::ref(gradient));
	value = v.v;
}
