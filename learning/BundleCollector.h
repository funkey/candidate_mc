#ifndef CANDIDATE_MC_LEARNING_BUNDLE_COLLECTOR_H__
#define CANDIDATE_MC_LEARNING_BUNDLE_COLLECTOR_H__

#include "LinearConstraints.h"

class BundleCollector {

public:

	template <typename Weights>
	void addHyperplane(const Weights& a, double b);

	/**
	 * Get all constraints that have been added so far.
	 */
	const LinearConstraints& getConstraints() const { return _constraints; }

	/**
	 * Get all constraints that have been added to this collector since the last 
	 * call to getNewConstraints().
	 */
	LinearConstraints getNewConstraints() {

		LinearConstraints newConstraints = _newConstraints;
		_newConstraints.clear();

		return newConstraints;
	}

private:

	LinearConstraints _constraints;
	LinearConstraints _newConstraints;
};

template <typename Weights>
void
BundleCollector::addHyperplane(const Weights& a, double b) {
	/*
	  <w,a> + b ≤  ξ
	        <=>
	  <w,a> - ξ ≤ -b
	*/

	unsigned int dims = a.size();

	LinearConstraint constraint;

	for (unsigned int i = 0; i < dims; i++)
		constraint.setCoefficient(i, a[i]);
	constraint.setCoefficient(dims, -1.0);
	constraint.setRelation(LessEqual);
	constraint.setValue(-b);

	_constraints.add(constraint);
	_newConstraints.add(constraint);
}

#endif // CANDIDATE_MC_LEARNING_BUNDLE_COLLECTOR_H__

