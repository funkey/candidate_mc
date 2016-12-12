#ifndef CANDIDATE_MC_IO_HDF5_VOLUME_READER_H__
#define CANDIDATE_MC_IO_HDF5_VOLUME_READER_H__

#include <string>
#include <imageprocessing/ExplicitVolume.h>
#include "Hdf5FileAccessor.h"

class Hdf5VolumeReader : public Hdf5FileAccessor {

public:

	Hdf5VolumeReader(std::string filename) :
		Hdf5FileAccessor(filename, vigra::HDF5File::OpenMode::ReadOnly) {}

	template <typename ValueType>
	void readVolume(ExplicitVolume<ValueType>& volume, std::string dataset) {

		readVolume(volume, dataset, false /* onlyGeometry */);
	}

	template <typename ValueType>
	void readVolume(ExplicitVolume<ValueType>& volume, std::string dataset, bool onlyGeometry) {

		// the volume
		if (!onlyGeometry)
			_hdfFile.readAndResize(dataset, volume.data());

		vigra::MultiArray<1, float> p(3);

		// resolution
		if (_hdfFile.existsAttribute(dataset, "resolution")) {

			_hdfFile.readAttribute(
					dataset,
					"resolution",
					p);
			volume.setResolution(p[0], p[1], p[2]);
		}

		// offset
		if (_hdfFile.existsAttribute(dataset, "offset")) {

			_hdfFile.readAttribute(
					dataset,
					"offset",
					p);
			volume.setOffset(p[0], p[1], p[2]);
		}
	}
};

#endif // CANDIDATE_MC_IO_HDF5_VOLUME_READER_H__

