#ifndef CANDIDATE_MC_CRAG_DOWN_SAMPLER_H__
#define CANDIDATE_MC_CRAG_DOWN_SAMPLER_H__

#include <map>
#include "Crag.h"
#include "CragVolumes.h"

class DownSampler {

public:

	/**
	 * Create a new CRAG downsampler. Candidates with a size smaller than 
	 * minSize will be removed, and single children contracted with their 
	 * parents.
	 */
	DownSampler(int minSize = -1) :
		_minSize(minSize) {}

	/**
	 * Downsample the given CRAG.
	 *
	 * @param[in]  crag
	 *                    The CRAG to downsample.
	 * @param[in]  volumes
	 *                    The volumes of the CRAG to downsample.
	 * @param[out] downSampled
	 *                    The downsampled CRAG.
	 * @param[out] downSampledVolumes
	 *                    The volumes of the downsampled CRAG.
	 */
	void process(const Crag& crag, const CragVolumes& volumes, Crag& downSampled, CragVolumes& downSampledVolumes);

private:

	void downSampleCopy(const Crag& source, const CragVolumes& volumes, Crag::CragNode parent, Crag::CragNode n, bool singleChild, Crag& target);

	int size(const Crag& crag, const CragVolumes& volumes, Crag::CragNode n);

	int _minSize;

	std::map<Crag::CragNode, Crag::CragNode> _copyMap;
	std::map<Crag::CragNode, int> _sizes;
};

#endif // CANDIDATE_MC_CRAG_DOWN_SAMPLER_H__

