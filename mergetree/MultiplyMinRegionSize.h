#ifndef MULTI2CUT_MERGETREE_MULTIPLY_MIN_REGION_SIZE_H__
#define MULTI2CUT_MERGETREE_MULTIPLY_MIN_REGION_SIZE_H__

#include <util/cont_map.hpp>
#include <util/ProgramOptions.h>
#include <util/assert.h>
#include "NodeNumConverter.h"

extern util::ProgramOption optionMultiplyMinRegionSizeExponent;

/**
 * A scoring function that multiplies the size of the smaller region with the 
 * score of another scoring function.
 */
template <typename ScoringFunctionType>
class MultiplyMinRegionSize {

public:

	static const int Dim = ScoringFunctionType::Dim;

	typedef typename ScoringFunctionType::GridGraphType GridGraphType;
	typedef typename ScoringFunctionType::RagType       RagType;

	typedef util::cont_map<typename RagType::Node, std::size_t, NodeNumConverter<RagType> > RegionSizesType;
	typedef util::cont_map<typename RagType::Node, float, NodeNumConverter<RagType> >       AverageIntensitiesType;

	template <typename T>
	MultiplyMinRegionSize(
			RagType&             rag,
			const T&             initialRegions,
			ScoringFunctionType& scoringFunction) :
		_rag(rag),
		_regionSizes(_rag),
		_scoringFunction(scoringFunction),
		_exponent(optionMultiplyMinRegionSizeExponent) {

		// get initial region sizes
		for (auto id : initialRegions)
			_regionSizes[_rag.nodeFromId(id)]++;
	}

	float operator()(const typename RagType::Edge& edge, std::vector<typename GridGraphType::Edge>& gridEdges) {

		typename RagType::Node u = _rag.u(edge);
		typename RagType::Node v = _rag.v(edge);

		float score = _scoringFunction(edge, gridEdges);

		score *= pow(std::min(_regionSizes[u], _regionSizes[v]), _exponent);

		return score;
	}

	void onMerge(const typename RagType::Edge& edge, const typename RagType::Node newRegion) {

		typename RagType::Node u = _rag.u(edge);
		typename RagType::Node v = _rag.v(edge);

		_regionSizes[newRegion] =
				_regionSizes[u] +
				_regionSizes[v];

		_scoringFunction.onMerge(edge, newRegion);
	}

private:

	RagType&             _rag;
	RegionSizesType      _regionSizes;
	ScoringFunctionType& _scoringFunction;

	float _exponent;
};

#endif // MULTI2CUT_MERGETREE_MULTIPLY_MIN_REGION_SIZE_H__


