#ifndef MULTI2CUT_MERGETREE_SMALL_FIRST_H__
#define MULTI2CUT_MERGETREE_SMALL_FIRST_H__

#include <util/cont_map.hpp>
#include <util/ProgramOptions.h>
#include <util/assert.h>
#include "NodeNumConverter.h"
#include "ScoringFunction.h"

extern util::ProgramOption optionSmallRegionThreshold1;
extern util::ProgramOption optionSmallRegionThreshold2;
extern util::ProgramOption optionIntensityThreshold;

/**
 * A scoring function that merges small regions first, that are:
 *
 *   below a size threshold t1
 *   below a size threshold t2 > t1, and have mean intensity (edgeness) above i
 *
 * Small regions and all other regions are merged in order of the given merging 
 * function.
 */
template <typename ScoringFunctionType>
class SmallFirst : public ScoringFunction {

public:

	typedef vigra::GridGraph<2>                                                    GridGraphType;
	typedef vigra::AdjacencyListGraph                                              RagType;
	typedef util::cont_map<RagType::Node, std::size_t, NodeNumConverter<RagType> > RegionSizesType;
	typedef util::cont_map<RagType::Node, float, NodeNumConverter<RagType> >       AverageIntensitiesType;

	/**
	 * Amount to subtract from small region scores, to make sure they are merged 
	 * first. This implies that the used scoring function must not exceed this 
	 * value.
	 */
	static const float Offset;

	SmallFirst(
			RagType&                              rag,
			const vigra::MultiArrayView<2, float> intensities,
			const vigra::MultiArrayView<2, int>   initialRegions,
			ScoringFunctionType&                  scoringFunction) :
		_rag(rag),
		_regionSizes(_rag),
		_averageIntensities(_rag),
		_intensities(intensities),
		_scoringFunction(scoringFunction),
		_t1(optionSmallRegionThreshold1),
		_t2(optionSmallRegionThreshold2),
		_i(optionIntensityThreshold) {

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

		float score = _scoringFunction(edge, gridEdges);

		UTIL_ASSERT_REL(score, >=, 0);
		UTIL_ASSERT_REL(score, <,  Offset);

		if (smallRegionEdge(edge))
			return score - Offset;

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

	bool smallRegionEdge(const RagType::Edge& edge) const {

		RagType::Node u = _rag.u(edge);
		RagType::Node v = _rag.v(edge);
		RagType::Node smaller;
		std::size_t minSize;

		if (_regionSizes[u] < _regionSizes[v]) {

			smaller = u;
			minSize = _regionSizes[u];

		} else {

			smaller = v;
			minSize = _regionSizes[v];
		}

		if (minSize < _t1)
			return true;

		if (minSize < _t2 && _averageIntensities[smaller] > _i)
			return true;

		return false;
	}

	RagType&               _rag;
	RegionSizesType        _regionSizes;
	AverageIntensitiesType _averageIntensities;

	vigra::MultiArrayView<2, float> _intensities;

	ScoringFunctionType& _scoringFunction;

	// thresholds
	int _t1, _t2;
	float _i;
};

template <typename ScoringFunctionType>
const float SmallFirst<ScoringFunctionType>::Offset = 1e3;

#endif // MULTI2CUT_MERGETREE_SMALL_FIRST_H__

