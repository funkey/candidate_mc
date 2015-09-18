#ifndef MULTI2CUT_MERGETREE_NODE_NUM_CONVERTER_H__
#define MULTI2CUT_MERGETREE_NODE_NUM_CONVERTER_H__

/**
 * A cont_map index adaptor that converts GraphType::Node into an integral type.
 */
template <typename GraphType>
struct NodeNumConverter {

	typedef typename GraphType::index_type TargetType;

	NodeNumConverter(GraphType& graph) :
		graph(graph) {}

	TargetType operator()(const typename GraphType::Node& node) const {
		return graph.id(node);
	}

	typename GraphType::Node operator()(const TargetType& id) const {
		return graph.nodeFromId(id);
	}

	GraphType& graph;
};

#endif // MULTI2CUT_MERGETREE_NODE_NUM_CONVERTER_H__

