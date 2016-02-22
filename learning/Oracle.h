#ifndef CANDIDATE_MC_LEARNING_ORACLE_H__
#define CANDIDATE_MC_LEARNING_ORACLE_H__

#include <vector>

/**
 * Base class for Oracles. Provides method stubs, to be overwritten by 
 * subclasses.
 *
 * The oracle is suppossed to provide an objective of the form
 *
 *   L(w) = P(w) - R(w),
 *
 * where P and R are convex functions in w.
 */
template <typename FeatureWeights>
class Oracle {

public:

	/**
	 * Evaluate P at w and return the value and gradient.
	 */
	virtual void valueGradientP(
			const FeatureWeights& w,
			double&               value,
			FeatureWeights&       gradient) = 0;

	/**
	 * Return the gradient of -R at w.
	 */
	virtual void valueGradientR(
			const FeatureWeights& weights,
			double&               value,
			FeatureWeights&       gradient) {

		// default implementation assumes that R is zero
		value = 0;
		gradient.importFromVector(std::vector<double>(weights.exportToVector().size(), 0));
	}

	/**
	 * Indicate whether R is unequal 0.
	 */
	virtual bool haveConcavePart() const { return false; }
};

#endif // CANDIDATE_MC_LEARNING_ORACLE_H__
