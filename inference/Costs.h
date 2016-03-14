#ifndef CANDIDATE_MC_INFERENCE_COSTS_H__
#define CANDIDATE_MC_INFERENCE_COSTS_H__

#include <crag/Crag.h>

class Costs {

public:

	Costs(const Crag& crag) : node(crag), edge(crag) {}

	/**
	 * Propagate values of the leaf nodes and edges upwards, such that
	 * different solutions resulting in the same segmentation have the same
	 * value. This function assumes that the leaf node and leaf edge values
	 * have been set. A leaf edge is an edge between two leaf nodes.
	 */
	void propagateLeafValues(const Crag& crag);

	/**
	 * Same as propagateLeafValues(), but only for edges.
	 */
	void propagateLeafEdgeValues(const Crag& crag);

	Crag::NodeMap<double> node;
	Crag::EdgeMap<double> edge;

private:

	void collectLeafNodes(
			const Crag&                                         crag,
			Crag::CragNode                                      n,
			std::map<Crag::CragNode, std::set<Crag::CragNode>>& leafNodes);

	double nodeValueFromLeafNodes(
			const Crag&                                               crag,
			const Crag::CragNode&                                     n,
			const std::map<Crag::CragNode, std::set<Crag::CragNode>>& leafNodes);

	double edgeValueFromLeafNodes(
			const Crag&                                               crag,
			const Crag::CragEdge&                                     e,
			const std::map<Crag::CragNode, std::set<Crag::CragNode>>& leafNodes);
};

#endif // CANDIDATE_MC_INFERENCE_COSTS_H__

