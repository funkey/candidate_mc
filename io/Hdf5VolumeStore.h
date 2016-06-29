#ifndef CANDIDATE_MC_IO_HDF5_VOLUME_STORE_H__
#define CANDIDATE_MC_IO_HDF5_VOLUME_STORE_H__

#include "VolumeStore.h"
#include "Hdf5VolumeReader.h"
#include "Hdf5VolumeWriter.h"

class Hdf5VolumeStore : public VolumeStore, public Hdf5VolumeReader, public Hdf5VolumeWriter {

public:

	Hdf5VolumeStore(std::string projectFile) :
		Hdf5VolumeReader(_hdfFile),
		Hdf5VolumeWriter(_hdfFile),
		_hdfFile(
				projectFile,
				vigra::HDF5File::OpenMode::ReadWrite) {}

	void saveIntensities(const ExplicitVolume<float>& intensities) override;

	void saveBoundaries(const ExplicitVolume<float>& boundaries) override;

	void saveGroundTruth(const ExplicitVolume<int>& groundTruth) override;

	void saveLabels(const ExplicitVolume<int>& labels) override;

	void retrieveIntensities(ExplicitVolume<float>& intensities) override;

	void retrieveBoundaries(ExplicitVolume<float>& boundaries) override;

	void retrieveGroundTruth(ExplicitVolume<int>& groundTruth) override;

	void retrieveLabels(ExplicitVolume<int>& labels) override;

	void retrieveVolume(ExplicitVolume<int>& volume, std::string name) {

		readVolume(volume, std::string("/volumes/") + name);
	}

private:

	vigra::HDF5File _hdfFile;
};

#endif // CANDIDATE_MC_IO_HDF5_VOLUME_STORE_H__

