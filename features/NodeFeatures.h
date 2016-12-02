#ifndef CANDIDATE_MC_FEATURES_NODE_FEATURES_H__
#define CANDIDATE_MC_FEATURES_NODE_FEATURES_H__

#include "Features.h"
#include "FeatureWeights.h"

class NodeFeatures {

	typedef Features<Crag::CragNode> FeaturesType;

public:

	NodeFeatures(const Crag& crag) :
			_crag(crag),
			_features(Crag::NodeTypes.size(), FeaturesType(crag)) {}

	inline void append(Crag::CragNode n, double feature) {

		features(_crag.type(n)).append(n, feature);
	}

	inline void appendFeatureName(Crag::NodeType type, std::string name) {

		features(type).appendFeatureName(name);
	}

	inline void appendFeatureNames(Crag::NodeType type, std::vector<std::string> names) {

		features(type).appendFeatureNames(names);
	}

	inline const std::vector<std::string>& getFeatureNames(Crag::NodeType type) const {

		return features(type).getFeatureNames();
	}

	const std::vector<double>& operator[](Crag::CragNode n) const {

		return features(_crag.type(n))[n];
	}

	void set(Crag::CragNode n, const std::vector<double>& v) {

		features(_crag.type(n)).set(n, v);
	}

	inline unsigned int dims(Crag::NodeType type) const {

		return features(type).dims();
	}

	void normalize() {

		for (auto& f : _features)
			f.normalize();
	}

	void normalize(
			const FeatureWeights& min,
			const FeatureWeights& max) {

		for (Crag::NodeType type : Crag::NodeTypes)
			features(type).normalize(min[type], max[type]);
	}

	void getMin(FeatureWeights& min) {

		for (Crag::NodeType type : Crag::NodeTypes)
			min[type] = features(type).getMin();
	}

	void getMax(FeatureWeights& max) {

		for (Crag::NodeType type : Crag::NodeTypes)
			max[type] = features(type).getMax();
	}

private:

	inline FeaturesType& features(Crag::NodeType type) {

		return _features[type];
	}

	inline const FeaturesType& features(Crag::NodeType type) const {

		return _features[type];
	}

	const Crag& _crag;

	// one set of features for each node type
	std::vector<FeaturesType> _features;
};

#endif // CANDIDATE_MC_FEATURES_NODE_FEATURES_H__

