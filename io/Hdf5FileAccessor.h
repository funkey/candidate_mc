#ifndef CANDIDATE_MC_IO_HDF5_FILE_ACCESSOR_H__
#define CANDIDATE_MC_IO_HDF5_FILE_ACCESSOR_H__

#include <vigra/hdf5impex.hxx>

class Hdf5FileAccessor {

protected:

	Hdf5FileAccessor(
			std::string filename,
			vigra::HDF5File::OpenMode mode) :
		_hdfFile(filename, mode) {}

	void cd(std::string path) {

		_hdfFile.cd(path);
	}

protected:

	vigra::HDF5File _hdfFile;
};

#endif // CANDIDATE_MC_IO_HDF5_FILE_ACCESSOR_H__

