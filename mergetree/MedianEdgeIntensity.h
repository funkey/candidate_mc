#ifndef MULTI2CUT_MERGETREE_MEDIAN_EDGE_INTENSITY_H__
#define MULTI2CUT_MERGETREE_MEDIAN_EDGE_INTENSITY_H__

#include <util/cont_map.hpp>
#include <vigra/graph_algorithms.hxx>
#include "EdgeNumConverter.h"

/**
 * An edge scoring function that returns the median intensity of the edge 
 * pixels.
 */
template <int D>
class MedianEdgeIntensity {

public:
	static const int Dim = D;

	typedef vigra::GridGraph<Dim>                           GridGraphType;
	typedef vigra::AdjacencyListGraph                       RagType;
	typedef typename GridGraphType::template EdgeMap<float> EdgeWeightsType;

	struct EdgeComp {

		EdgeComp(const EdgeWeightsType& weights_)
			: weights(weights_) {}

		bool operator()(
				const typename GridGraphType::Edge& a,
				const typename GridGraphType::Edge& b) const {
			return weights[a] < weights[b];
		}

		const EdgeWeightsType& weights;
	};

	MedianEdgeIntensity(const vigra::MultiArrayView<D, float> intensities) :
		_grid(intensities.shape()),
		_edgeWeights(_grid) {

		vigra::edgeWeightsFromNodeWeights(
				_grid,
				intensities,
				_edgeWeights);

		if (_edgeWeights.size() == 0)
			return;
	}

	/**
	 * Get the score for an edge. An edge will be merged the earlier, the 
	 * smaller its score is.
	 */
	float operator()(const RagType::Edge& edge, std::vector<typename GridGraphType::Edge>& gridEdges) {

		typename std::vector<typename GridGraphType::Edge>::iterator median = gridEdges.begin() + gridEdges.size()/2;
		std::nth_element(gridEdges.begin(), median, gridEdges.end(), EdgeComp(_edgeWeights));

		return _edgeWeights[*median];
	}

	void onMerge(const typename RagType::Edge&, const typename RagType::Node) {}

private:

	GridGraphType   _grid;
	EdgeWeightsType _edgeWeights;
};

#endif // MULTI2CUT_MERGETREE_MEDIAN_EDGE_INTENSITY_H__

