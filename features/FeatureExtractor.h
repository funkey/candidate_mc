#ifndef CANDIDATE_MC_FEATURES_FEATURE_EXTRACTOR_H__
#define CANDIDATE_MC_FEATURES_FEATURE_EXTRACTOR_H__

#include <io/CragStore.h>
#include "NodeFeatures.h"
#include "EdgeFeatures.h"
#include "FeatureProvider.h"

class FeatureExtractor {

public:

	FeatureExtractor(
			Crag&        crag,
			CragVolumes& volumes) :
		_crag(crag),
		_volumes(volumes),
		_numOriginalVolumeNodeFeatures(0),
		_numOriginalSliceNodeFeatures(0),
		_numOriginalAssignmentNodeFeatures(0),
		_numOriginalEdgeFeatures(0){}

	/**
	 * Extract node and edge features, along with the respective min and max 
	 * values that have been used for normalization. If the provided min and max 
	 * are not empty, they will be used for normalizing the features (instead of 
	 * computing min and max from the extracted features). Use this for a 
	 * testing dataset where you want to make sure that the features are 
	 * normalized in the same way as in the training dataset.
	 */
	void extract(
			FeatureProviderBase& featureProvider,
			NodeFeatures& nodeFeatures,
			EdgeFeatures& edgeFeatures);

	void normalize(
			NodeFeatures& nodeFeatures,
			EdgeFeatures& edgeFeatures,
			FeatureWeights& min,
			FeatureWeights& max);

private:

	void extractNodeFeatures(
			FeatureProviderBase& featureProvider,
			NodeFeatures& nodeFeatures);

	void extractEdgeFeatures(
			FeatureProviderBase& featureProvider,
			const NodeFeatures& nodeFeatures,
			EdgeFeatures& edgeFeatures);

	Crag&        _crag;
	CragVolumes& _volumes;

	// number of "real" node features, before add squares and bias
	int _numOriginalVolumeNodeFeatures;
	int _numOriginalSliceNodeFeatures;
	int _numOriginalAssignmentNodeFeatures;

	// number of "real" edge features, before add squares and bias
	int _numOriginalEdgeFeatures;
};

#endif // CANDIDATE_MC_FEATURES_FEATURE_EXTRACTOR_H__

