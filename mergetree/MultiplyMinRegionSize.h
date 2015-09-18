#ifndef MULTI2CUT_MERGETREE_MULTIPLY_MIN_REGION_SIZE_H__
#define MULTI2CUT_MERGETREE_MULTIPLY_MIN_REGION_SIZE_H__

#include <util/cont_map.hpp>
#include <util/ProgramOptions.h>
#include <util/assert.h>
#include "NodeNumConverter.h"
#include "ScoringFunction.h"

extern util::ProgramOption optionMultiplyMinRegionSizeExponent;

/**
 * A scoring function that multiplies the size of the smaller region with the 
 * score of another scoring function.
 */
template <typename ScoringFunctionType>
class MultiplyMinRegionSize : public ScoringFunction {

public:

	typedef vigra::GridGraph<2>                                                    GridGraphType;
	typedef vigra::AdjacencyListGraph                                              RagType;
	typedef util::cont_map<RagType::Node, std::size_t, NodeNumConverter<RagType> > RegionSizesType;
	typedef util::cont_map<RagType::Node, float, NodeNumConverter<RagType> >       AverageIntensitiesType;

	MultiplyMinRegionSize(
			RagType&                              rag,
			const vigra::MultiArrayView<2, float> intensities,
			const vigra::MultiArrayView<2, int>   initialRegions,
			ScoringFunctionType&                  scoringFunction) :
		_rag(rag),
		_regionSizes(_rag),
		_averageIntensities(_rag),
		_intensities(intensities),
		_scoringFunction(scoringFunction),
		_exponent(optionMultiplyMinRegionSizeExponent) {

		// get initial region sizes and average intensities
		vigra::MultiArray<2, int>::iterator   i = initialRegions.begin();
		vigra::MultiArray<2, float>::iterator j = intensities.begin();

		UTIL_ASSERT(initialRegions.shape() == intensities.shape());

		for (; i != initialRegions.end(); i++, j++) {

			RagType::Node node = _rag.nodeFromId(*i);
			float value = *j;

			_regionSizes[node]++;
			_averageIntensities[node] += value;
		}

		for (RagType::NodeIt node(_rag); node != lemon::INVALID; ++node)
			_averageIntensities[*node] /= _regionSizes[*node];
	}

	float operator()(const RagType::Edge& edge, std::vector<GridGraphType::Edge>& gridEdges) {

		RagType::Node u = _rag.u(edge);
		RagType::Node v = _rag.v(edge);

		float score = _scoringFunction(edge, gridEdges);

		score *= pow(std::min(_regionSizes[u], _regionSizes[v]), _exponent);

		return score;
	}

	void onMerge(const RagType::Edge& edge, const RagType::Node newRegion) {

		RagType::Node u = _rag.u(edge);
		RagType::Node v = _rag.v(edge);

		_regionSizes[newRegion] =
				_regionSizes[u] +
				_regionSizes[v];

		_averageIntensities[newRegion] =
				(_averageIntensities[u]*_regionSizes[u]+
				 _averageIntensities[v]*_regionSizes[v])/
				(_regionSizes[u] + _regionSizes[v]);

		_scoringFunction.onMerge(edge, newRegion);
	}

private:

	RagType&               _rag;
	RegionSizesType        _regionSizes;
	AverageIntensitiesType _averageIntensities;

	vigra::MultiArrayView<2, float> _intensities;

	ScoringFunctionType& _scoringFunction;

	float _exponent;
};

#endif // MULTI2CUT_MERGETREE_MULTIPLY_MIN_REGION_SIZE_H__


