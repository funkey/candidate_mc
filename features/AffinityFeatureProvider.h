#ifndef CANDIDATE_MC_FEATURES_AFFINITY_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_AFFINITY_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>
#include <boost/accumulators/statistics/p_square_quantile.hpp>

class AffinityFeatureProvider : public FeatureProvider<AffinityFeatureProvider> {

public:

	AffinityFeatureProvider(
			const Crag& crag,
			const ExplicitVolume<float>& xAffinities,
			const ExplicitVolume<float>& yAffinities,
			const ExplicitVolume<float>& zAffinities,
			const std::string valuesName = "affinities") :
		_crag(crag),
		_xAffinities(xAffinities),
		_yAffinities(yAffinities),
		_zAffinities(zAffinities),
		_valuesName(valuesName){}

	template <typename ContainerT>
	void appendEdgeFeatures(const Crag::CragEdge e, ContainerT& adaptor) {

		if (_crag.type(e) == Crag::AdjacencyEdge)
		{
			using namespace boost::accumulators;
			typedef  stats<
				tag::min,
				tag::median,
				tag::max,
				tag::mean,
				tag::moment<2>,
				tag::moment<3>
			> Stats;
			accumulator_set<double, Stats> accumulator;


			typedef accumulator_set<double, stats<tag::p_square_quantile>> quantile_accumulator;
			quantile_accumulator acc25(quantile_probability = 0.25);
			quantile_accumulator acc75(quantile_probability = 0.75);

			const auto & gridGraph = _crag.getGridGraph();

			// push data into the accumulator
			unsigned int numAffiliatedEdges = 0;
			for (Crag::CragEdge leafEdge : _crag.leafEdges(e))
				for (vigra::GridGraph<3>::Edge ae : _crag.getAffiliatedEdges(leafEdge)) {

					const auto ggU = gridGraph.u(ae);
					const auto ggV = gridGraph.v(ae);

					auto max = std::max(ggU, ggV);
					auto min = std::min(ggU, ggV);

					float affinity;
					if (max[0] != min[0])
						affinity = _xAffinities[max];
					else if (max[1] != min[1])
						affinity = _yAffinities[max];
					else
						affinity = _zAffinities[max];

					accumulator(affinity);
					acc25(affinity);
					acc75(affinity);

					numAffiliatedEdges++;
				}

			// TODO: affilitatedEdgesProvider?
			adaptor.append(numAffiliatedEdges);

			// extract the features from the accumulator
			adaptor.append(min(accumulator));
			adaptor.append(p_square_quantile(acc25));
			adaptor.append(median(accumulator));
			adaptor.append(p_square_quantile(acc75));
			adaptor.append(max(accumulator));
			adaptor.append(mean(accumulator));
			adaptor.append(sqrt(moment<2>(accumulator)));
			adaptor.append(moment<3>(accumulator));
		}
	}

	std::map<Crag::EdgeType, std::vector<std::string>> getEdgeFeatureNames() const override {

		std::map<Crag::EdgeType, std::vector<std::string>> names;

		names[Crag::AdjacencyEdge].push_back("num_affiliated_edges_" + _valuesName);
		names[Crag::AdjacencyEdge].push_back("affinities_min_" + _valuesName);
		names[Crag::AdjacencyEdge].push_back("affinities_25quantile_" + _valuesName);
		names[Crag::AdjacencyEdge].push_back("affinities_median_" + _valuesName);
		names[Crag::AdjacencyEdge].push_back("affinities_75quantile_" + _valuesName);
		names[Crag::AdjacencyEdge].push_back("affinities_max_" + _valuesName);
		names[Crag::AdjacencyEdge].push_back("affinities_mean_" + _valuesName);
		names[Crag::AdjacencyEdge].push_back("affinities_stddev_" + _valuesName);
		names[Crag::AdjacencyEdge].push_back("affinities_skew_" + _valuesName);

		return names;
	}

private:

	const Crag& _crag;
	const ExplicitVolume<float>& _xAffinities;
	const ExplicitVolume<float>& _yAffinities;
	const ExplicitVolume<float>& _zAffinities;
	std::string _valuesName;
};

#endif // CANDIDATE_MC_FEATURES_AFFINITY_FEATURE_PROVIDER_H__

