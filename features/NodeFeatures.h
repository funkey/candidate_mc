#ifndef CANDIDATE_MC_FEATURES_NODE_FEATURES_H__
#define CANDIDATE_MC_FEATURES_NODE_FEATURES_H__

#include <vector>
#include <util/exceptions.h>
#include "Crag.h"

class NodeFeatures : public Crag::NodeMap<std::vector<double>> {

public:

	/**
	 * Create feature map for the given CRAG.
	 */
	NodeFeatures(Crag& crag) :
			Crag::NodeMap<std::vector<double>>(crag),
			_crag(crag) {}

	/**
	 * Create a new feature map and reserve enough memory to fit the given 
	 * number of features per node.
	 */
	NodeFeatures(Crag& crag, unsigned int numFeatures) :
			Crag::NodeMap<std::vector<double>>(crag),
			_crag(crag) {

		for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
			(*this)[n].reserve(numFeatures);
	}

	/**
	 * Add a single feature to the feature vector for a node. Converts nan into 
	 * 0.
	 */
	inline void append(Crag::Node n, double feature) {

		if (feature != feature)
			feature = 0;

		(*this)[n].push_back(feature);
	}

	/**
	 * The size of the feature vectors.
	 */
	inline unsigned int dims() const {

		Crag::NodeIt n(_crag);

		if (n == lemon::INVALID)
			return 0;

		return (*this)[n].size();
	}

	/**
	 * Normalize all features, such that they are in the range [0,1]. The min 
	 * and max values used for the transformation can be queried with getMin() 
	 * and getMax().
	 */
	void normalize() {

		findMinMax();
		normalizeMinMax(_min, _max);
	}

	/**
	 * Normalize all features, but instead of searching for the min and max, use 
	 * the provided ones. This will also set the min and max returned by 
	 * getMin() and getMax().
	 */
	void normalize(
			const std::vector<double>& min,
			const std::vector<double>& max) {

		_min = min;
		_max = max;
		normalizeMinMax(_min, _max);
	}

	/**
	 * Get the minimal values of the features.
	 */
	const std::vector<double>& getMin() {

		if (_min.size() == 0)
			findMinMax();

		return _min;
	}

	/**
	 * Get the maximal values of the features.
	 */
	const std::vector<double>& getMax() {

		if (_max.size() == 0)
			findMinMax();

		return _max;
	}

private:

	void findMinMax() {

		_min.clear();
		_max.clear();

		for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

			const std::vector<double>& features = (*this)[n];

			if (_min.size() == 0) {

				_min = features;
				_max = features;

			} else {

				for (unsigned int i = 0; i < _min.size(); i++) {

					_min[i] = std::min(_min[i], features[i]);
					_max[i] = std::max(_max[i], features[i]);
				}
			}
		}
	}

	void normalizeMinMax(
			const std::vector<double>& min,
			const std::vector<double>& max) {

		if (min.size() != max.size())
			UTIL_THROW_EXCEPTION(
					UsageError,
					"provided min and max have different sizes");

		if (min.size() != dims())
			UTIL_THROW_EXCEPTION(
					UsageError,
					"provided min and max have different size " << min.size() << " than features " << dims());

		for (Crag::NodeIt n(_crag); n != lemon::INVALID; ++n) {

			std::vector<double>& features = (*this)[n];

			if (features.size() != min.size())
				UTIL_THROW_EXCEPTION(
						UsageError,
						"encountered a feature with differnt size " << features.size() << " than given min " << min.size());

			for (unsigned int i = 0; i < min.size(); i++) {

				if (max[i] - min[i] <= 1e-10)
					continue;

				features[i] = (features[i] - min[i])/(max[i] - min[i]);
			}
		}
	}

	const Crag& _crag;

	std::vector<double> _min, _max;
};

#endif // CANDIDATE_MC_FEATURES_NODE_FEATURES_H__

