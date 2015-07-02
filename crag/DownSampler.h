#ifndef CANDIDATE_MC_CRAG_DOWN_SAMPLER_H__
#define CANDIDATE_MC_CRAG_DOWN_SAMPLER_H__

#include "Crag.h"

class DownSampler {

public:

	/**
	 * Create a new CRAG downsampler. Candidates with a size smaller than 
	 * minSize will be removed, and single children contracted with their 
	 * parents.
	 */
	DownSampler(int minSize) :
		_minSize(minSize) {}

	/**
	 * Process the given CRAG.
	 */
	void process(const Crag& crag, Crag* downSampled);

private:

	void downSampleCopy(const Crag& source, Crag::SubsetNode parent, Crag::SubsetNode n, bool singleChild, Crag& target);

	int size(const Crag& crag, Crag::SubsetNode n);

	int _minSize;

	std::map<Crag::SubsetNode, Crag::Node> _copyMap;
	std::map<Crag::SubsetNode, int> _sizes;
};

#endif // CANDIDATE_MC_CRAG_DOWN_SAMPLER_H__

