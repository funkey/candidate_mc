#ifndef MULTI2CUT_MERGETREE_MULTIPLY_SIZE_DIFFERENCE_SIZE_H__
#define MULTI2CUT_MERGETREE_MULTIPLY_SIZE_DIFFERENCE_SIZE_H__

#include <util/cont_map.hpp>
#include <util/ProgramOptions.h>
#include <util/assert.h>
#include "NodeNumConverter.h"
#include "ScoringFunction.h"

extern util::ProgramOption optionMultiplySizeDifferenceExponent;

/**
 * A scoring function that multiplies the size of the smaller region with the 
 * score of another scoring function.
 */
template <typename ScoringFunctionType>
class MultiplySizeDifference : public ScoringFunction {

public:

	typedef vigra::GridGraph<2>                                                    GridGraphType;
	typedef vigra::AdjacencyListGraph                                              RagType;
	typedef util::cont_map<RagType::Node, std::size_t, NodeNumConverter<RagType> > RegionSizesType;

	MultiplySizeDifference(
			RagType&                              rag,
			const vigra::MultiArrayView<2, int>   initialRegions,
			ScoringFunctionType&                  scoringFunction) :
		_rag(rag),
		_regionSizes(_rag),
		_scoringFunction(scoringFunction),
		_exponent(optionMultiplySizeDifferenceExponent) {

		// get initial region sizes
		vigra::MultiArray<2, int>::const_iterator   i = initialRegions.begin();

		for (; i != initialRegions.end(); i++) {

			RagType::Node node = _rag.nodeFromId(*i);
			_regionSizes[node]++;
		}
	}

	float operator()(const RagType::Edge& edge, std::vector<GridGraphType::Edge>& gridEdges) {

		RagType::Node u = _rag.u(edge);
		RagType::Node v = _rag.v(edge);

		float score = _scoringFunction(edge, gridEdges);

		score *= pow(std::abs(_regionSizes[u] - _regionSizes[v]), _exponent);

		return score;
	}

	void onMerge(const RagType::Edge& edge, const RagType::Node newRegion) {

		RagType::Node u = _rag.u(edge);
		RagType::Node v = _rag.v(edge);

		_regionSizes[newRegion] =
				_regionSizes[u] +
				_regionSizes[v];

		_scoringFunction.onMerge(edge, newRegion);
	}

private:

	RagType&               _rag;
	RegionSizesType        _regionSizes;

	ScoringFunctionType& _scoringFunction;

	float _exponent;
};

#endif // MULTI2CUT_MERGETREE_MULTIPLY_SIZE_DIFFERENCE_H__

