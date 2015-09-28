#ifndef CANDIDATE_MC_FEATURES_FEATURE_WEIGHTS_H__
#define CANDIDATE_MC_FEATURES_FEATURE_WEIGHTS_H__

#include <vector>
#include <map>
#include <crag/Crag.h>
#include <io/vectors.h>
#include <util/assert.h>

class NodeFeatures;
class EdgeFeatures;

class FeatureWeights {

public:

	/**
	 * Create an empty set of feature weights.
	 */
	FeatureWeights();

	/**
	 * Create feature weights suitable for the passed features, and initialize 
	 * them uniformly.
	 */
	FeatureWeights(const NodeFeatures& nodeFeatures, const EdgeFeatures& edgeFeatures, double value);

	/**
	 * Get the feature weight vector for a node type.
	 */
	const std::vector<double>& operator[](Crag::NodeType type) const { return _nodeFeatureWeights.at(type); }
	std::vector<double>& operator[](Crag::NodeType type) { return _nodeFeatureWeights[type]; }

	/**
	 * Get the feature weight vector for an edge type.
	 */
	const std::vector<double>& operator[](Crag::EdgeType type) const { return _edgeFeatureWeights.at(type); }
	std::vector<double>& operator[](Crag::EdgeType type) { return _edgeFeatureWeights[type]; }

	/**
	 * Overwrite the current weights with the given value.
	 */
	void fill(double value) {

		for (auto& p : _nodeFeatureWeights)
			for (double& v : p.second)
				v = value;
		for (auto& p : _edgeFeatureWeights)
			for (double& v : p.second)
				v = value;
	}

	/**
	 * Test if feautre weights have been set. This function returns true as soon 
	 * as any of the feature weight vectors is non-empty.
	 */
	inline bool empty() const {

		for (auto& p : _nodeFeatureWeights)
			if (p.second.size() > 0)
				return false;
		for (auto& p : _edgeFeatureWeights)
			if (p.second.size() > 0)
				return false;
		return true;
	}

	/**
	 * Create a vector with all the feature weights. To be used by classes that 
	 * don't care about the internal structure of the parameters (like 
	 * BundleOptimizer and GradientOptimizer).
	 */
	std::vector<double> exportToVector() const {

		std::vector<double> v;

		for (auto& p : _nodeFeatureWeights)
			std::copy(p.second.begin(), p.second.end(), std::back_inserter(v));
		for (auto& p : _edgeFeatureWeights)
			std::copy(p.second.begin(), p.second.end(), std::back_inserter(v));

		return v;
	}

	/**
	 * Read the feature weights from a vector congruent to the one exported by 
	 * exportToVector. To be used by classes that don't care about the internal 
	 * structure of the parameters (like BundleOptimizer and GradientOptimizer).
	 */
	void importFromVector(const std::vector<double>& v) {

		auto b = v.begin();

		for (auto& p : _nodeFeatureWeights) {

			std::copy(b, b + p.second.size(), p.second.begin());
			b += p.second.size();
		}

		for (auto& p : _edgeFeatureWeights) {

			std::copy(b, b + p.second.size(), p.second.begin());
			b += p.second.size();
		}

		UTIL_ASSERT(b == v.end());
	}

private:

	std::map<Crag::NodeType, std::vector<double>> _nodeFeatureWeights;
	std::map<Crag::EdgeType, std::vector<double>> _edgeFeatureWeights;
};

std::ostream&
operator<<(std::ostream& os, const FeatureWeights& weights);

#endif // CANDIDATE_MC_FEATURES_FEATURE_WEIGHTS_H__

