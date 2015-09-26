#include "FeatureWeights.h"
#include <features/NodeFeatures.h>
#include <features/EdgeFeatures.h>
#include <util/helpers.hpp>

FeatureWeights::FeatureWeights(const NodeFeatures& nodeFeatures, const EdgeFeatures& edgeFeatures, double value) {

	for (Crag::NodeType type : {Crag::VolumeNode, Crag::SliceNode, Crag::AssignmentNode})
		_nodeFeatureWeights[type].resize(nodeFeatures.dims(type), value);

	for (Crag::EdgeType type : {Crag::AdjacencyEdge, Crag::NoAssignmentEdge})
		_edgeFeatureWeights[type].resize(edgeFeatures.dims(type), value);
}

std::ostream&
operator<<(std::ostream& os, const FeatureWeights& weights) {

	for (auto type : {Crag::VolumeNode, Crag::SliceNode, Crag::AssignmentNode})
		os << "node type " << type << " " << weights[type] << std::endl;
	for (auto type : {Crag::AdjacencyEdge, Crag::NoAssignmentEdge})
		os << "edge type " << type << " " << weights[type] << std::endl;

	return os;
}
