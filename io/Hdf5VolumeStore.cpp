#include "Hdf5VolumeStore.h"

void
Hdf5VolumeStore::saveIntensities(const ExplicitVolume<float>& intensities) {

	writeVolume(intensities, "intensities");
}

void
Hdf5VolumeStore::saveBoundaries(const ExplicitVolume<float>& boundaries) {

	writeVolume(boundaries, "boundaries");
}

void
Hdf5VolumeStore::saveGroundTruth(const ExplicitVolume<int>& labels) {

	writeVolume(labels, "groundtruth");
}

void
Hdf5VolumeStore::saveAffinities(
		const ExplicitVolume<float>& xAffinities,
		const ExplicitVolume<float>& yAffinities,
		const ExplicitVolume<float>& zAffinities) {

	writeVolume(xAffinities, "xAffinities");
	writeVolume(yAffinities, "yAffinities");
	writeVolume(zAffinities, "zAffinities");

}

void
Hdf5VolumeStore::retrieveIntensities(ExplicitVolume<float>& intensities) {

	readVolume(intensities, "intensities");
}

void
Hdf5VolumeStore::retrieveBoundaries(ExplicitVolume<float>& boundaries) {

	readVolume(boundaries, "boundaries");
}

void
Hdf5VolumeStore::retrieveGroundTruth(ExplicitVolume<int>& labels) {

	readVolume(labels, "groundtruth");
}

void
Hdf5VolumeStore::retrieveAffinities(
		ExplicitVolume<float>& xAffinities,
		ExplicitVolume<float>& yAffinities,
		ExplicitVolume<float>& zAffinities) {

	readVolume(xAffinities, "xAffinities");
	readVolume(yAffinities, "yAffinities");
	readVolume(zAffinities, "zAffinities");
}
