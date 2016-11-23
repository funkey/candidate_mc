#ifndef CANDIDATE_MC_FEATURES_ACCUMULATED_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_ACCUMULATED_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>

class AccumulatedFeatureProvider : public FeatureProvider<AccumulatedFeatureProvider> {

public:

	AccumulatedFeatureProvider(
			const Crag& crag,
			const ExplicitVolume<float>& values,
			const std::string valuesName = "values") :
		_crag(crag),
		_values(values),
		_valuesName(valuesName){

	}

	template <typename ContainerT>
	void appendEdgeFeatures(const Crag::CragEdge e, ContainerT& adaptor) {

		if (_crag.type(e) == Crag::AdjacencyEdge)
		{
			// Define an accumulator set for calculating the mean and the
			// 2nd moment .
			using namespace boost::accumulators;
			typedef  stats<
				tag::mean,
				tag::moment<1>,
				tag::moment<2>
			> Stats;
			accumulator_set<double, Stats> accumulator;

			const auto & gridGraph = _crag.getGridGraph();

			// push data into the accumulator
			unsigned int numAffiliatedEdges = 0;
			for (Crag::CragEdge leafEdge : _crag.leafEdges(e))
				for (vigra::GridGraph<3>::Edge ae : _crag.getAffiliatedEdges(leafEdge)) {
					const auto ggU = gridGraph.u(ae);
					const auto ggV = gridGraph.v(ae);

					accumulator(_values[ggU]);
					accumulator(_values[ggV]);

					numAffiliatedEdges++;
				}

			// TODO: affilitatedEdgesProvider?
			adaptor.append(numAffiliatedEdges);

			// extract the features from the accumulator
			adaptor.append(mean(accumulator));
			adaptor.append(moment<1>(accumulator));
			adaptor.append(moment<2>(accumulator));

		}
	}

	std::map<Crag::EdgeType, std::vector<std::string>> getEdgeFeatureNames() const override {

		std::map<Crag::EdgeType, std::vector<std::string>> names;

		names[Crag::AdjacencyEdge].push_back("num_affiliated_edges_" + _valuesName);
		names[Crag::AdjacencyEdge].push_back("affiliated_edges_mean_" + _valuesName);
		names[Crag::AdjacencyEdge].push_back("affiliated_edges_stddev_" + _valuesName);
		names[Crag::AdjacencyEdge].push_back("affiliated_edges_skey_" + _valuesName);

		return names;
	}

private:

	const Crag& _crag;
	const ExplicitVolume<float>& _values;
	std::string _valuesName;
};

#endif // CANDIDATE_MC_FEATURES_ACCUMULATED_FEATURE_PROVIDER_H__

