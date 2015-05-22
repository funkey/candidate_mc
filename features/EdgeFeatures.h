#ifndef CANDIDATE_MC_FEATURES_EDGE_FEATURES_H__
#define CANDIDATE_MC_FEATURES_EDGE_FEATURES_H__

#include <vector>
#include "Crag.h"

class EdgeFeatures : public Crag::EdgeMap<std::vector<double>> {

public:

	/**
	 * Create feature map for the given CRAG.
	 */
	EdgeFeatures(Crag& crag) :
			Crag::EdgeMap<std::vector<double>>(crag) {}

	/**
	 * Create a new feature map and reserve enough memory to fit the given 
	 * number of features per edge.
	 */
	EdgeFeatures(Crag& crag, unsigned int numFeatures) :
			Crag::EdgeMap<std::vector<double>>(crag) {

		for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e)
			(*this)[e].reserve(numFeatures);
	}

	/**
	 * Add a single feature to the feature vector for a edge. Converts nan into 
	 * 0.
	 */
	inline void append(Crag::Edge e, double feature) {

		if (feature != feature)
			feature = 0;

		(*this)[e].push_back(feature);
	}
};

#endif // CANDIDATE_MC_FEATURES_EDGE_FEATURES_H__


