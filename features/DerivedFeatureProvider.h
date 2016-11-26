#ifndef CANDIDATE_MC_FEATURES_DERIVED_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_DERIVED_FEATURE_PROVIDER_H__

#include "FeatureProvider.h"
#include <util/assert.h>

class DerivedFeatureProvider : public FeatureProvider<DerivedFeatureProvider> {

public:

	DerivedFeatureProvider(
			const Crag& crag,
			const NodeFeatures& nodeFeatures) :
		_crag(crag),
		_nodeFeatures(nodeFeatures),
		_numOriginalNodeFeatures(0){}

	template <typename ContainerT>
	void appendEdgeFeatures(const Crag::CragEdge e, ContainerT& adaptor) {

		if (_crag.type(e) != Crag::AdjacencyEdge)
			return;

		for (Crag::CragEdge e : _crag.edges()) {

			if (_crag.type(e) != Crag::AdjacencyEdge)
				continue;

			switch (_crag.type(e.u())) {

				case Crag::VolumeNode:
					_numOriginalNodeFeatures = _nodeFeatures.dims(Crag::VolumeNode);
					break;
				case Crag::SliceNode:
					_numOriginalNodeFeatures = _nodeFeatures.dims(Crag::SliceNode);
					break;
				default:
					_numOriginalNodeFeatures = _nodeFeatures.dims(Crag::AssignmentNode);
			}
			break;
		}

		Crag::CragNode u = e.u();
		Crag::CragNode v = e.v();

		// feature vectors from node u/v
		const auto & featsU = _nodeFeatures[u];
		const auto & featsV = _nodeFeatures[v];

		// NOTE: All adjacency edges links two equal types nodes?
		UTIL_ASSERT(_crag.type(u) == _crag.type(v));
		UTIL_ASSERT_REL(featsU.size(), ==, featsV.size());
		UTIL_ASSERT_REL(featsU.size(), >=, _numOriginalNodeFeatures);
		UTIL_ASSERT_REL(featsV.size(), >=, _numOriginalNodeFeatures);

		// loop over all features
		for(size_t nfi=0; nfi < _numOriginalNodeFeatures; ++nfi){
			// single feature from node u/v
			const auto fu = featsU[nfi];
			const auto fv = featsV[nfi];
			// convert u/v features into
			// edge features
			adaptor.append(std::abs(fu-fv));
			adaptor.append(std::min(fu,fv));
			adaptor.append(std::max(fu,fv));
			adaptor.append(fu+fv);
		}
	}

	std::map<Crag::EdgeType, std::vector<std::string>> getEdgeFeatureNames() const override {

		std::map<Crag::EdgeType, std::vector<std::string>> names;

		std::cout << "num node features names" << _numOriginalNodeFeatures << std::endl;

		for (size_t i = 0; i < _numOriginalNodeFeatures; i++) {

			names[Crag::AdjacencyEdge].push_back(std::string("derived_node_abs_") + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back(std::string("derived_node_min_") + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back(std::string("derived_node_max_") + boost::lexical_cast<std::string>(i));
			names[Crag::AdjacencyEdge].push_back(std::string("derived_node_sum_") + boost::lexical_cast<std::string>(i));
		}

		return names;
	}

private:

	const Crag& _crag;

	// already extracted features
	const NodeFeatures& _nodeFeatures;
	int _numOriginalNodeFeatures;
};

#endif // CANDIDATE_MC_FEATURES_DERIVED_FEATURE_PROVIDER_H__

