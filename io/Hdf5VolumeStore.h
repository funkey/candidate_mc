#ifndef CANDIDATE_MC_IO_HDF5_VOLUME_STORE_H__
#define CANDIDATE_MC_IO_HDF5_VOLUME_STORE_H__

#include "VolumeStore.h"
#include "Hdf5VolumeReader.h"
#include "Hdf5VolumeWriter.h"

class Hdf5VolumeStore : public VolumeStore, public Hdf5VolumeReader, public Hdf5VolumeWriter {

public:

	Hdf5VolumeStore(std::string projectFile) :
		Hdf5VolumeReader(projectFile),
		Hdf5VolumeWriter(projectFile) {

		Hdf5VolumeReader::cd("/volumes");
		Hdf5VolumeWriter::cd("/volumes");
	}

	void saveIntensities(const ExplicitVolume<float>& intensities) override;

	void saveBoundaries(const ExplicitVolume<float>& boundaries) override;

	void saveGroundTruth(const ExplicitVolume<int>& groundTruth) override;

	void saveAffinities(const ExplicitVolume<float>& xAffinities,
						const ExplicitVolume<float>& yAffinities,
						const ExplicitVolume<float>& zAffinities) override;

	void retrieveIntensities(ExplicitVolume<float>& intensities) override;

	void retrieveBoundaries(ExplicitVolume<float>& boundaries) override;

	void retrieveGroundTruth(ExplicitVolume<int>& groundTruth) override;

	void retrieveAffinities(ExplicitVolume<float>& xAffinities,
							ExplicitVolume<float>& yAffinities,
							ExplicitVolume<float>& zAffinities) override;

	void retrieveVolume(ExplicitVolume<int>& volume, std::string name) {

		readVolume(volume, std::string("/volumes/") + name);
	}
};

#endif // CANDIDATE_MC_IO_HDF5_VOLUME_STORE_H__

