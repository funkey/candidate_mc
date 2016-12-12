#ifndef CANDIDATE_MC_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURE_PROVIDER_H__

class FeatureProviderBase {

public:

	virtual ~FeatureProviderBase() {}

	virtual void appendFeatures(
			const Crag& crag,
			NodeFeatures& nodeFeatures) = 0;

	virtual void appendFeatures(
			const Crag& crag,
			EdgeFeatures& edgeFeatures) = 0;
};

/**
 * Base class for feature providers, implements empty default methods.
 */
template <typename Derived>
class FeatureProvider : public FeatureProviderBase {

public:

	void appendFeatures(const Crag& crag, NodeFeatures& nodeFeatures) override {

		#pragma omp parallel
		for (auto n = crag.nodes().begin(); n != crag.nodes().end(); n++) {

			FeatureNodeAdaptor adaptor(nodeFeatures, *n);
			static_cast<Derived*>(this)->appendNodeFeatures(*n, adaptor);
		}

		for (const auto& p : getNodeFeatureNames())
			nodeFeatures.appendFeatureNames(p.first, p.second);
	}

	void appendFeatures(const Crag& crag, EdgeFeatures& edgeFeatures) override {

		#pragma omp parallel
		for (auto e = crag.edges().begin(); e != crag.edges().end(); e++) {

			FeatureEdgeAdaptor adaptor(edgeFeatures, *e);
			static_cast<Derived*>(this)->appendEdgeFeatures(*e, adaptor);
		}

		for (const auto& p : getEdgeFeatureNames())
			edgeFeatures.appendFeatureNames(p.first, p.second);
	}

	template <typename ContainerT>
	void appendNodeFeatures(const Crag::CragNode n, ContainerT& features) {}

	template <typename ContainerT>
	void appendEdgeFeatures(const Crag::CragEdge e, ContainerT& features) {}

	virtual std::map<Crag::NodeType, std::vector<std::string>> getNodeFeatureNames() const {

		return std::map<Crag::NodeType, std::vector<std::string>>();
	}

	virtual std::map<Crag::EdgeType, std::vector<std::string>> getEdgeFeatureNames() const {

		return std::map<Crag::EdgeType, std::vector<std::string>>();
	}

private:

	/**
	 * Adaptor to be used with RegionFeatures. Just appends to a vector, 
	 * ignoring the "id" of the region.
	 */
	class FeatureNodeAdaptor {

	public:
		FeatureNodeAdaptor(NodeFeatures& features, Crag::CragNode n) : _features(features), _n(n) {}

		inline void append(double value)                           { _features.append(_n, value); }
		inline void append(unsigned int /*ignored*/, double value) { _features.append(_n, value); }
		inline const std::vector<double>& getFeatures(){ return _features[_n]; }
		inline const std::vector<std::string> getFeatureNames(Crag::NodeType type){ return _features.getFeatureNames(type); }

	private:

		NodeFeatures&  _features;
		Crag::CragNode _n;
	};

	/**
	 * Adaptor to be used with RegionFeatures. Just appends to a vector, 
	 * ignoring the "id" of the region.
	 */
	class FeatureEdgeAdaptor {

	public:
		FeatureEdgeAdaptor(EdgeFeatures& features, Crag::CragEdge e) : _features(features), _e(e) {}

		inline void append(double value)                           { _features.append(_e, value); }
		inline void append(unsigned int /*ignored*/, double value) { _features.append(_e, value); }
		inline const std::vector<double>& getFeatures(){ return _features[_e]; }
		inline const std::vector<std::string> getFeatureNames(Crag::EdgeType type){ return _features.getFeatureNames(type); }

	private:

		EdgeFeatures&  _features;
		Crag::CragEdge _e;
	};
};

#endif // CANDIDATE_MC_FEATURE_PROVIDER_H__

