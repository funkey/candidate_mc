#ifndef CANDIDATE_MC_FEATURES_CONTACT_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_CONTACT_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"
#include "ContactFeature.h"

class ContactFeatureProvider : public FeatureProvider<ContactFeatureProvider> {

public:

	ContactFeatureProvider(
			const Crag& crag,
			const CragVolumes& volumes,
			const ExplicitVolume<float>& values,
			const std::string valuesName = "values") :
		_crag(crag),
		_volumes (volumes),
		_values(values),
		_valuesName(valuesName){
	}

	template <typename ContainerT>
	void appendEdgeFeatures(const Crag::CragEdge e, ContainerT& adaptor) {

		if (_crag.type(e) == Crag::AdjacencyEdge)
		{
			ContactFeature contactFeature(_crag, _volumes, _values);

			for (double feature : contactFeature.compute(e))
				adaptor.append(feature);
		}
	}

	std::map<Crag::EdgeType, std::vector<std::string>> getEdgeFeatureNames() const override {

		std::map<Crag::EdgeType, std::vector<std::string>> names;

		for (int i = 0; i < 3/*number of thresholds*/; i++)
		{
			names[Crag::AdjacencyEdge].push_back(_valuesName + "_contact_score_edge_u_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back(_valuesName + "_normalized_contact_score_edge_u_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back(_valuesName + "_contact_score_edge_v_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back(_valuesName + "_normalized_contact_score_edge_v_" + boost::lexical_cast<std::string>(i));

			names[Crag::AdjacencyEdge].push_back(_valuesName + "_log_contact_score_edge_u_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back(_valuesName + "_log_normalized_contact_score_edge_u_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back(_valuesName + "_log_contact_score_edge_v_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back(_valuesName + "_log_normalized_contact_score_edge_v_" + boost::lexical_cast<std::string>(i));
		}

		names[Crag::AdjacencyEdge].push_back(_valuesName + "_log_volume_ratio_u");
		names[Crag::AdjacencyEdge].push_back(_valuesName + "_log_volume_ratio_v");
		names[Crag::AdjacencyEdge].push_back(_valuesName + "_volume_ratio_u");
		names[Crag::AdjacencyEdge].push_back(_valuesName + "_volume_ratio_v");

		return names;
	}

private:

	const Crag&        _crag;
	const CragVolumes& _volumes;

	const ExplicitVolume<float>& _values;
	std::string _valuesName;
};

#endif // CANDIDATE_MC_FEATURES_CONTACT_FEATURE_PROVIDER_H__

