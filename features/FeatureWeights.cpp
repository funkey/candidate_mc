#include "FeatureWeights.h"
#include <features/NodeFeatures.h>
#include <features/EdgeFeatures.h>
#include <util/helpers.hpp>

FeatureWeights::FeatureWeights() {

	for (Crag::NodeType type : Crag::NodeTypes)
		_nodeFeatureWeights[type] = std::vector<double>();

	for (Crag::EdgeType type : Crag::EdgeTypes)
		_edgeFeatureWeights[type] = std::vector<double>();
}

FeatureWeights::FeatureWeights(const NodeFeatures& nodeFeatures, const EdgeFeatures& edgeFeatures, double value) {

	for (Crag::NodeType type : Crag::NodeTypes)
		_nodeFeatureWeights[type].resize(nodeFeatures.dims(type), value);

	for (Crag::EdgeType type : Crag::EdgeTypes)
		_edgeFeatureWeights[type].resize(edgeFeatures.dims(type), value);
}

std::ostream&
operator<<(std::ostream& os, const FeatureWeights& weights) {

	for (Crag::NodeType type : Crag::NodeTypes)
		os << "node type " << type << " " << weights[type] << std::endl;
	for (Crag::EdgeType type : Crag::EdgeTypes)
		os << "edge type " << type << " " << weights[type] << std::endl;

	return os;
}
