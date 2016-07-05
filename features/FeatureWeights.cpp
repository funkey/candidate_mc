#include <algorithm>
#include "FeatureWeights.h"
#include <features/NodeFeatures.h>
#include <features/EdgeFeatures.h>
#include <util/assert.h>
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

void
FeatureWeights::mask(const FeatureWeights& mask) {

	UTIL_ASSERT_REL(exportToVector().size(), ==, mask.exportToVector().size());

	auto mask_op = [](double x, double m) { return (m == 0 ? 0 : x ); };

	for (Crag::NodeType type : Crag::NodeTypes)
		std::transform(
				_nodeFeatureWeights[type].begin(),
				_nodeFeatureWeights[type].end(),
				mask._nodeFeatureWeights.at(type).begin(),
				_nodeFeatureWeights[type].begin(),
				mask_op);

	for (Crag::EdgeType type : Crag::EdgeTypes)
		std::transform(
				_edgeFeatureWeights[type].begin(),
				_edgeFeatureWeights[type].end(),
				mask._edgeFeatureWeights.at(type).begin(),
				_edgeFeatureWeights[type].begin(),
				mask_op);
}

std::ostream&
operator<<(std::ostream& os, const FeatureWeights& weights) {

	for (Crag::NodeType type : Crag::NodeTypes)
		os << "node type " << type << " " << weights[type] << std::endl;
	for (Crag::EdgeType type : Crag::EdgeTypes)
		os << "edge type " << type << " " << weights[type] << std::endl;

	return os;
}
