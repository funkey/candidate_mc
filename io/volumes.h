#ifndef CANDIDATE_MC_IO_VOLUMES_H__
#define CANDIDATE_MC_IO_VOLUMES_H__

#include <vigra/impex.hxx>
#include <imageprocessing/ExplicitVolume.h>
#include <util/exceptions.h>

template <typename T>
ExplicitVolume<T> readVolume(std::vector<std::string> filenames) {

	if (filenames.size() == 0) {

		LOG_ERROR(logger::out) << "no files" << std::endl;
		return ExplicitVolume<T>();
	}

	int depth = filenames.size();

	std::string filename = filenames[0];
	vigra::ImageImportInfo info = vigra::ImageImportInfo(filename.c_str());
	ExplicitVolume<T> volume(info.width(), info.height(), depth);

	for (int z = 0; z < depth; z++) {

		try {

			vigra::ImageImportInfo info = vigra::ImageImportInfo(filenames[z].c_str());
			importImage(info, volume.data().template bind<2>(z));

		} catch (std::exception& e) {

			UTIL_THROW_EXCEPTION(
					IOError,
					"error reading " << filenames[z] << ": " << e.what());
		}
	}

	return volume;
}

std::vector<std::string>
getImageFiles(std::string path);

#endif // CANDIDATE_MC_IO_VOLUMES_H__

