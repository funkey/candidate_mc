#ifndef CANDIDATE_MC_FEATURES_AFFINITY_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_AFFINITY_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>

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
				tag::max,
				tag::mean,
				tag::moment<2>,
				tag::moment<3>
			> Stats;
			accumulator_set<double, Stats> accumulator;
			std::vector<double> affinities;
			const auto & gridGraph = _crag.getGridGraph();

			for (Crag::CragEdge leafEdge : _crag.leafEdges(e))
				for (vigra::GridGraph<3>::Edge ae : _crag.getAffiliatedEdges(leafEdge)) {

					const auto ggU = gridGraph.u(ae);
					const auto ggV = gridGraph.v(ae);

					auto max = std::max(ggU, ggV);
					auto min = std::min(ggU, ggV);

					double affinity;
					if (max[0] != min[0])
						affinity = _xAffinities[max];
					else if (max[1] != min[1])
						affinity = _yAffinities[max];
					else
						affinity = _zAffinities[max];

					accumulator(affinity);
					affinities.push_back(affinity);
				}

			// TODO: affilitatedEdgesProvider?

			// number of affiliated edges
			adaptor.append(affinities.size());

			// extract the features from the accumulator
			// quartiles and median are approximation on accumulator, so we will get information from std::nth_element
			auto const quantile25 = affinities.size() / 4;
			auto const median     = affinities.size() / 2;
			auto const quantile75 = quantile25 + median;

			// quantile 25%
			std::nth_element(affinities.begin(),                  affinities.begin() + quantile25, affinities.end());
			//median
			std::nth_element(affinities.begin() + quantile25 + 1, affinities.begin() + median,     affinities.end());
			// quantile 75%
			std::nth_element(affinities.begin() + median + 1,     affinities.begin() + quantile75, affinities.end());

			adaptor.append(min(accumulator));
			adaptor.append(affinities[quantile25]);
			adaptor.append(affinities[median]);
			adaptor.append(affinities[quantile75]);
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

