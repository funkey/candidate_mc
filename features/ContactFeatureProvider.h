#ifndef CANDIDATE_MC_FEATURES_CONTACT_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_CONTACT_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"
#include "ContactFeature.h"

class ContactFeatureProvider : public FeatureProvider<ContactFeatureProvider> {

public:

	ContactFeatureProvider(
			const Crag& crag,
			const CragVolumes& volumes,
			const ExplicitVolume<float>& values) :
		_crag(crag),
		_volumes (volumes),
		_values(values){
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
			names[Crag::AdjacencyEdge].push_back("contact_score_edge_u_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back("normalized_contact_score_edge_u_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back("contact_score_edge_v_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back("normalized_contact_score_edge_v_" + boost::lexical_cast<std::string>(i));

			names[Crag::AdjacencyEdge].push_back("log_contact_score_edge_u_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back("log_normalized_contact_score_edge_u_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back("log_contact_score_edge_v_" + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back("log_normalized_contact_score_edge_v_" + boost::lexical_cast<std::string>(i));
		}

		names[Crag::AdjacencyEdge].push_back("log_volume_ratio_u");
		names[Crag::AdjacencyEdge].push_back("log_volume_ratio_v");
		names[Crag::AdjacencyEdge].push_back("volume_ratio_u");
		names[Crag::AdjacencyEdge].push_back("volume_ratio_v");

		return names;
	}

private:

	const Crag&        _crag;
	const CragVolumes& _volumes;

	const ExplicitVolume<float>& _values;
};

#endif // CANDIDATE_MC_FEATURES_TOPOLOGICAL_FEATURE_PROVIDER_H__

