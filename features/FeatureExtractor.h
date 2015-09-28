#ifndef CANDIDATE_MC_FEATURES_FEATURE_EXTRACTOR_H__
#define CANDIDATE_MC_FEATURES_FEATURE_EXTRACTOR_H__

#include <io/CragStore.h>
#include "NodeFeatures.h"
#include "EdgeFeatures.h"

class FeatureExtractor {

public:

	FeatureExtractor(
			Crag&                        crag,
			CragVolumes&                 volumes,
			const ExplicitVolume<float>& raw,
			const ExplicitVolume<float>& boundaries) :
		_crag(crag),
		_volumes(volumes),
		_raw(raw),
		_boundaries(boundaries) {}

	/**
	 * Set a list of sample segmentations. A segmentation is a list of sets of 
	 * CRAG leaf nodes. If set, those will be used to compute edge affinities 
	 * based on the number of times the adjacent candidates have been part of a 
	 * segment.
	 */
	void setSampleSegmentations(const std::vector<std::vector<std::set<Crag::Node>>>& sampleSegmentations) {

		_segmentations = sampleSegmentations;
	}

	/**
	 * Extract node and edge features, along with the respective min and max 
	 * values that have been used for normalization. If the provided min and max 
	 * are not empty, they will be used for normalizing the features (instead of 
	 * computing min and max from the extracted features). Use this for a 
	 * testing dataset where you want to make sure that the features are 
	 * normalized in the same way as in the training dataset.
	 */
	void extract(
			NodeFeatures& nodeFeatures,
			EdgeFeatures& edgeFeatures,
			FeatureWeights& min,
			FeatureWeights& max);

private:

	/**
	 * Adaptor to be used with RegionFeatures, such that mapping to the correct 
	 * node is preserved.
	 */
	class FeatureNodeAdaptor {

	public:
		FeatureNodeAdaptor(Crag::CragNode node, NodeFeatures& features) : _node(node), _features(features) {}

		inline void append(unsigned int /*ignored*/, double value) { _features.append(_node, value); }

	private:

		Crag::CragNode _node;
		NodeFeatures&  _features;
	};

	void extractNodeFeatures(NodeFeatures& nodeFeatures, FeatureWeights& min, FeatureWeights& max);

	void extractEdgeFeatures(const NodeFeatures& nodeFeatures, EdgeFeatures& edgeFeatures, FeatureWeights& min, FeatureWeights& max);

	void extractNodeShapeFeatures(NodeFeatures& nodeFeatures);

	void extractDerivedEdgeFeatures(const NodeFeatures& nodeFeatures, EdgeFeatures& edgeFeatures);

    void extractAccumulatedEdgeFeatures(EdgeFeatures& edgeFeatures);

	void extractEdgeSegmentationFeatures(EdgeFeatures& edgeFeatures);

	void extractTopologicalNodeFeatures(NodeFeatures& nodeFeatures);

	void extractNodeStatisticsFeatures(NodeFeatures& nodeFeatures);

	std::pair<int, int> recExtractTopologicalFeatures(NodeFeatures& nodeFeatures, Crag::CragNode n);

	void visualizeEdgeFeatures(const EdgeFeatures& edgeFeatures);

	Crag&        _crag;
	CragVolumes& _volumes;

	const ExplicitVolume<float>& _raw;
	const ExplicitVolume<float>& _boundaries;

	std::vector<std::vector<std::set<Crag::Node>>> _segmentations;

	// number of "real" node features, before add squares and bias
	int _numOriginalVolumeNodeFeatures;

	// number of "real" edge features, before add squares and bias
	int _numOriginalEdgeFeatures;

	bool _useProvidedMinMax;
};

#endif // CANDIDATE_MC_FEATURES_FEATURE_EXTRACTOR_H__

