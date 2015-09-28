#ifndef CANDIDATE_MC_FEATURES_FEATURES_H__
#define CANDIDATE_MC_FEATURES_FEATURES_H__

#include <vector>
#include <util/exceptions.h>
#include "Crag.h"

template <typename KeyType>
class Features {

public:

	Features(const Crag& crag) : _crag(crag), _dimsDirty(true) {}

	/**
	 * Add a single feature to the feature vector for a node. Converts nan into 
	 * 0.
	 */
	inline void append(KeyType n, double feature) {

		if (feature != feature)
			feature = 0;

		_features[n].push_back(feature);

		_dimsDirty = true;
	}

	/**
	 * Explicitly set a feature vector.
	 */
	inline void set(KeyType n, const std::vector<double>& v) {

		_features[n] = v;
		_dimsDirty = true;
	}

	/**
	 * The size of the feature vectors.
	 */
	inline unsigned int dims() const {

		if (!_dimsDirty)
			return _dims;

		_dims = 0;

		bool first = true;
		int firstId;
		for (auto& p : _features) {

			KeyType n = p.first;
			const std::vector<double>& f = p.second;

			if (first) {

				_dims = f.size();
				first = false;
				firstId = _crag.id(n);

			} else {

				if (f.size() != _dims)
					UTIL_THROW_EXCEPTION(
							UsageError,
							"Features contains vectors of different sizes: "
							"expected " << _dims << " (as seen for id " << firstId << ")" <<
							", found " << f.size() << " for id " << _crag.id(n));
			}
		}

		_dimsDirty = false;
		return _dims;
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

	const std::vector<double>& operator[](KeyType k) const { return _features.at(k); }

protected:

	// don't allow non-const access to feature vectors
	std::vector<double>& operator[](KeyType k) { return _features[k]; }

private:

	void findMinMax() {

		_min.clear();
		_max.clear();

		for (auto& p : _features) {

			const std::vector<double>& f = p.second;

			if (_min.size() == 0) {

				_min = f;
				_max = f;

			} else {

				for (unsigned int i = 0; i < _min.size(); i++) {

					_min[i] = std::min(_min[i], f[i]);
					_max[i] = std::max(_max[i], f[i]);
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

		for (auto& p : _features) {

			std::vector<double>& f = p.second;

			if (f.size() != min.size())
				UTIL_THROW_EXCEPTION(
						UsageError,
						"encountered a feature with differnt size " << f.size() << " than given min " << min.size());

			for (unsigned int i = 0; i < min.size(); i++) {

				if (max[i] - min[i] <= 1e-10)
					continue;

				f[i] = (f[i] - min[i])/(max[i] - min[i]);
			}
		}
	}

	const Crag& _crag;

	std::map<KeyType, std::vector<double>> _features;

	std::vector<double> _min, _max;

	mutable unsigned int _dims;
	mutable bool         _dimsDirty;
};

#endif // CANDIDATE_MC_FEATURES_FEATURES_H__

