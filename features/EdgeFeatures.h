#ifndef CANDIDATE_MC_FEATURES_EDGE_FEATURES_H__
#define CANDIDATE_MC_FEATURES_EDGE_FEATURES_H__

#include "Features.h"
#include "FeatureWeights.h"

class EdgeFeatures {

	typedef Features<Crag::CragEdge> FeaturesType;

public:

	EdgeFeatures(const Crag& crag) :
			_crag(crag),
			_features(Crag::EdgeTypes.size(), FeaturesType(crag)) {}

	inline void append(Crag::CragEdge e, double feature) {

		features(_crag.type(e)).append(e, feature);
	}

	inline void appendFeatureName(Crag::EdgeType type, std::string name) {

		features(type).appendFeatureName(name);
	}

	inline void appendFeatureNames(Crag::EdgeType type, std::vector<std::string> names) {

		features(type).appendFeatureNames(names);
	}

	inline std::vector<std::string> getFeatureNames(Crag::EdgeType type) const {

		return features(type).getFeatureNames();
	}

	const std::vector<double>& operator[](Crag::CragEdge e) const {

		return features(_crag.type(e))[e];
	}

	void set(Crag::CragEdge e, const std::vector<double>& v) {

		features(_crag.type(e)).set(e, v);
	}

	inline unsigned int dims(Crag::EdgeType type) const {

		return features(type).dims();
	}

	void normalize() {

		for (auto& f : _features)
			f.normalize();
	}

	void normalize(
			const FeatureWeights& min,
			const FeatureWeights& max) {

		for (Crag::EdgeType type : Crag::EdgeTypes)
			features(type).normalize(min[type], max[type]);
	}

	void getMin(FeatureWeights& min) {

		for (Crag::EdgeType type : Crag::EdgeTypes)
			min[type] = features(type).getMin();
	}

	void getMax(FeatureWeights& max) {

		for (Crag::EdgeType type : Crag::EdgeTypes)
			max[type] = features(type).getMax();
	}

private:

	inline FeaturesType& features(Crag::EdgeType type) {

		return _features[type];
	}

	const FeaturesType& features(Crag::EdgeType type) const {

		return _features[type];
	}

	const Crag& _crag;

	std::vector<FeaturesType> _features;
};

#endif // CANDIDATE_MC_FEATURES_EDGE_FEATURES_H__

