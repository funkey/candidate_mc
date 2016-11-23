#ifndef CANDIDATE_MC_COMPOSITE_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_COMPOSITE_FEATURE_PROVIDER_H__

#include <util/typename.h>

#include "FeatureProvider.h"

class CompositeFeatureProvider : public FeatureProviderBase {

public:

	void appendFeatures(
			const Crag& crag,
			NodeFeatures& nodeFeatures) override {

		for (FeatureProviderBase* provider : _providers)
			provider->appendFeatures(crag, nodeFeatures);
	}

	void appendFeatures(
			const Crag& crag,
			EdgeFeatures& edgeFeatures) override {

		for (FeatureProviderBase* provider : _providers)
			provider->appendFeatures(crag, edgeFeatures);
	}

	template <typename ProviderType, typename... Args>
	void emplace_back(Args&&... args) {

		ProviderType* provider = new ProviderType(std::forward<Args>(args)...);
		_providers.push_back(provider);
	}

	~CompositeFeatureProvider() {

		for (FeatureProviderBase* provider : _providers)
			delete provider;
	}

private:

	std::vector<FeatureProviderBase*> _providers;
};

#endif // CANDIDATE_MC_COMPOSITE_FEATURE_PROVIDER_H__

