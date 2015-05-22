#ifndef CANDIDATE_MC_FEATURES_NODE_FEATURES_H__
#define CANDIDATE_MC_FEATURES_NODE_FEATURES_H__

#include <vector>
#include "Crag.h"

class NodeFeatures : public Crag::NodeMap<std::vector<double>> {

public:

	/**
	 * Create feature map for the given CRAG.
	 */
	NodeFeatures(Crag& crag) :
			Crag::NodeMap<std::vector<double>>(crag) {}

	/**
	 * Create a new feature map and reserve enough memory to fit the given 
	 * number of features per node.
	 */
	NodeFeatures(Crag& crag, unsigned int numFeatures) :
			Crag::NodeMap<std::vector<double>>(crag) {

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
};

#endif // CANDIDATE_MC_FEATURES_NODE_FEATURES_H__

