#include "PyOracle.h"

void
PyOracle::valueGradientP(
		const PyOracleWeights& weights,
		double&                value,
		PyOracleWeights&       gradient) {

	// forward the oracle call to the python callback
	Value v;
	_callbackP(weights, boost::ref(v), boost::ref(gradient));
	value = v.v;
}

void
PyOracle::valueGradientR(
		const PyOracleWeights& weights,
		double&                value,
		PyOracleWeights&       gradient) {

	Value v;
	_callbackR(weights, boost::ref(v), boost::ref(gradient));
	value = v.v;
}
