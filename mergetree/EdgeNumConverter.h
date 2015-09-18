#ifndef MULTI2CUT_MERGETREE_EDGE_NUM_CONVERTER_H__
#define MULTI2CUT_MERGETREE_EDGE_NUM_CONVERTER_H__

/**
 * A cont_map index adaptor that converts GraphType::Edge into an integral type.
 */
template <typename GraphType>
struct EdgeNumConverter {

	typedef typename GraphType::index_type TargetType;

	EdgeNumConverter(GraphType& graph) :
		graph(graph) {}

	TargetType operator()(const typename GraphType::Edge& edge) const {
		return graph.id(edge);
	}

	typename GraphType::Edge operator()(const TargetType& id) const {
		return graph.edgeFromId(id);
	}

	GraphType& graph;
};

#endif // MULTI2CUT_MERGETREE_EDGE_NUM_CONVERTER_H__

