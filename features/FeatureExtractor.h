#ifndef CANDIDATE_MC_FEATURES_FEATURE_EXTRACTOR_H__
#define CANDIDATE_MC_FEATURES_FEATURE_EXTRACTOR_H__

#include <io/CragStore.h>
#include "NodeFeatures.h"
#include "EdgeFeatures.h"

class FeatureExtractor {

public:

	FeatureExtractor(
			Crag&                        crag,
			const ExplicitVolume<float>& raw,
			const ExplicitVolume<float>& boundaries) :
		_crag(crag),
		_raw(raw),
		_boundaries(boundaries) {}

	void extract(
			NodeFeatures& nodeFeatures,
			EdgeFeatures& edgeFeatures);

private:

	/**
	 * Adaptor to be used with RegionFeatures, such that mapping to the correct 
	 * node is preserved.
	 */
	class FeatureNodeAdaptor {

	public:
		FeatureNodeAdaptor(Crag::Node node, NodeFeatures& features) : _node(node), _features(features) {}

		inline void append(unsigned int /*ignored*/, double value) { _features.append(_node, value); }

	private:

		Crag::Node    _node;
		NodeFeatures& _features;
	};

	void extractNodeFeatures(NodeFeatures& nodeFeatures);

	void extractEdgeFeatures(const NodeFeatures& nodeFeatures, EdgeFeatures& edgeFeatures);

	std::pair<int, int> extractTopologicalFeatures(NodeFeatures& nodeFeatures, Crag::SubsetNode n);

	util::box<float, 3> getBoundingBox(Crag::Node n);

	void recFill(
			const util::box<float, 3>&           boundingBox,
			vigra::MultiArray<3, unsigned char>& labelImage,
			Crag::Node                           n);

	Crag& _crag;

	const ExplicitVolume<float>& _raw;
	const ExplicitVolume<float>& _boundaries;
};

#endif // CANDIDATE_MC_FEATURES_FEATURE_EXTRACTOR_H__

