#ifndef CANDIDATE_MC_IO_VOLUME_STORE_H__
#define CANDIDATE_MC_IO_VOLUME_STORE_H__

#include <imageprocessing/ExplicitVolume.h>

/**
 * Interface definition for volume stores.
 */
class VolumeStore {

public:

	/**
	 * Store the given intensity volume.
	 */
	virtual void saveIntensities(const ExplicitVolume<float>& intensities) = 0;

	/**
	 * Store the given boundary prediction volume.
	 */
	virtual void saveBoundaries(const ExplicitVolume<float>& boundaries) = 0;

	/**
	 * Store the given ground-truth label volume.
	 */
	virtual void saveGroundTruth(const ExplicitVolume<int>& groundTruth) = 0;

	/**
	 * Store the given label volume.
	 */
	virtual void saveLabels(const ExplicitVolume<int>& labels) = 0;

	/**
	 * Store the given affinities.
	 */
	virtual void saveAffinities(const ExplicitVolume<float>& xAffinities,
								const ExplicitVolume<float>& yAffinities,
								const ExplicitVolume<float>& zAffinities) = 0;

	/**
	 * Get the intensity volume.
	 */
	virtual void retrieveIntensities(ExplicitVolume<float>& intensities) = 0;

	/**
	 * Get the boundary prediction volume.
	 */
	virtual void retrieveBoundaries(ExplicitVolume<float>& boundaries) = 0;

	/**
	 * Get the ground-truth label volume.
	 */
	virtual void retrieveGroundTruth(ExplicitVolume<int>& groundTruth) = 0;

	/**
	 * Get the label volume.
	 */
	virtual void retrieveLabels(ExplicitVolume<int>& labels) = 0;

	/**
	 * Get the affinities volumes.
	 */
	virtual void retrieveAffinities(ExplicitVolume<float>& xAffinities,
									ExplicitVolume<float>& yAffinities,
									ExplicitVolume<float>& zAffinities) = 0;

	/**
	 * Get a volume by its name.
	 */
	virtual void retrieveVolume(ExplicitVolume<int>& volume, std::string name) = 0;
};


#endif // CANDIDATE_MC_IO_VOLUME_STORE_H__

