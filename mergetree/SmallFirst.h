#ifndef MULTI2CUT_MERGETREE_SMALL_FIRST_H__
#define MULTI2CUT_MERGETREE_SMALL_FIRST_H__

#include <util/cont_map.hpp>
#include <util/ProgramOptions.h>
#include <util/assert.h>
#include <vigra/multi_array.hxx>
#include <vigra/graph_algorithms.hxx>
#include "NodeNumConverter.h"

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
class SmallFirst {

public:

	static const int Dim = ScoringFunctionType::Dim;

	typedef typename ScoringFunctionType::GridGraphType GridGraphType;
	typedef typename ScoringFunctionType::RagType       RagType;

	typedef util::cont_map<typename RagType::Node, std::size_t, NodeNumConverter<RagType> > RegionSizesType;
	typedef util::cont_map<typename RagType::Node, float, NodeNumConverter<RagType> >       AverageIntensitiesType;

	/**
	 * Amount to subtract from small region scores, to make sure they are merged 
	 * first. This implies that the used scoring function must not exceed this 
	 * value.
	 */
	static const float Offset;

	SmallFirst(
			RagType&                                rag,
			const vigra::MultiArrayView<Dim, float> intensities,
			const vigra::MultiArrayView<Dim, int>   initialRegions,
			ScoringFunctionType&                    scoringFunction) :
		_rag(rag),
		_regionSizes(_rag),
		_averageIntensities(_rag),
		_intensities(intensities),
		_scoringFunction(scoringFunction),
		_t1(optionSmallRegionThreshold1),
		_t2(optionSmallRegionThreshold2),
		_i(optionIntensityThreshold) {

		// get initial region sizes and average intensities
		typename vigra::MultiArray<Dim, int>::const_iterator   i = initialRegions.begin();
		typename vigra::MultiArray<Dim, float>::const_iterator j = intensities.begin();

		UTIL_ASSERT(initialRegions.shape() == intensities.shape());

		for (; i != initialRegions.end(); i++, j++) {

			typename RagType::Node node = _rag.nodeFromId(*i);
			float value = *j;

			_regionSizes[node]++;
			_averageIntensities[node] += value;
		}

		for (typename RagType::NodeIt node(_rag); node != lemon::INVALID; ++node)
			_averageIntensities[*node] /= _regionSizes[*node];
	}

	float operator()(const typename RagType::Edge& edge, std::vector<typename GridGraphType::Edge>& gridEdges) {

		float score = _scoringFunction(edge, gridEdges);

		UTIL_ASSERT_REL(score, >=, 0);
		UTIL_ASSERT_REL(score, <,  Offset);

		if (smallRegionEdge(edge))
			return score - Offset;

		return score;
	}

	void onMerge(const typename RagType::Edge& edge, const typename RagType::Node newRegion) {

		typename RagType::Node u = _rag.u(edge);
		typename RagType::Node v = _rag.v(edge);

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

	bool smallRegionEdge(const typename RagType::Edge& edge) const {

		typename RagType::Node u = _rag.u(edge);
		typename RagType::Node v = _rag.v(edge);
		typename RagType::Node smaller;
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

	vigra::MultiArrayView<Dim, float> _intensities;

	ScoringFunctionType& _scoringFunction;

	// thresholds
	int _t1, _t2;
	float _i;
};

template <typename ScoringFunctionType>
const float SmallFirst<ScoringFunctionType>::Offset = 1e3;

#endif // MULTI2CUT_MERGETREE_SMALL_FIRST_H__

