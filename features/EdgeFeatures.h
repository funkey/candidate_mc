#ifndef CANDIDATE_MC_FEATURES_EDGE_FEATURES_H__
#define CANDIDATE_MC_FEATURES_EDGE_FEATURES_H__

#include "Features.h"

class EdgeFeatures {

	typedef Features<Crag::CragEdge> FeaturesType;

public:

	EdgeFeatures(const Crag& crag) :
			_crag(crag),
			_adFeatures(crag),
			_naFeatures(crag) {}

	inline void append(Crag::CragEdge e, double feature) {

		features(_crag.type(e)).append(e, feature);
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

		features(Crag::AdjacencyEdge).normalize();
		features(Crag::NoAssignmentEdge).normalize();
	}

	void normalize(
			const FeatureWeights& min,
			const FeatureWeights& max) {

		features(Crag::AdjacencyEdge).normalize(min[Crag::AdjacencyEdge], max[Crag::AdjacencyEdge]);
		features(Crag::NoAssignmentEdge).normalize(min[Crag::NoAssignmentEdge], max[Crag::NoAssignmentEdge]);
	}

	void getMin(FeatureWeights& min) {

		min[Crag::AdjacencyEdge] = features(Crag::AdjacencyEdge).getMin();
		min[Crag::NoAssignmentEdge] = features(Crag::NoAssignmentEdge).getMin();
	}

	void getMax(FeatureWeights& max) {

		max[Crag::AdjacencyEdge] = features(Crag::AdjacencyEdge).getMax();
		max[Crag::NoAssignmentEdge] = features(Crag::NoAssignmentEdge).getMax();
	}

private:

	FeaturesType& features(Crag::EdgeType type) {

		switch (type) {

			case Crag::AdjacencyEdge:
				return _adFeatures;
			case Crag::NoAssignmentEdge:
				return _naFeatures;
			case Crag::AssignmentEdge:
				UTIL_THROW_EXCEPTION(
						UsageError,
						"edges of type AssignmentEdge don't have features");
		}

		UTIL_THROW_EXCEPTION(
				UsageError,
				"unknown edge type " << type);
	}

	const FeaturesType& features(Crag::EdgeType type) const {

		switch (type) {

			case Crag::AdjacencyEdge:
				return _adFeatures;
			case Crag::NoAssignmentEdge:
				return _naFeatures;
			case Crag::AssignmentEdge:
				UTIL_THROW_EXCEPTION(
						UsageError,
						"edges of type AssignmentEdge don't have features");
		}

		UTIL_THROW_EXCEPTION(
				UsageError,
				"unknown edge type " << type);
	}

	const Crag& _crag;

	FeaturesType _adFeatures;
	FeaturesType _naFeatures;
};

#endif // CANDIDATE_MC_FEATURES_EDGE_FEATURES_H__

