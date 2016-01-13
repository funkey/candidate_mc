#include "VolumeRays.h"
#include <util/geometry.hpp>

void
VolumeRays::extractFromVolumes(const CragVolumes& volumes, float sampleRadius, float sampleDensity) {

	_sampleRadius  = sampleRadius;
	_sampleDensity = sampleDensity;

	for (Crag::CragNode n : _crag.nodes())
		extract(n, *volumes[n]);
}

void
VolumeRays::extract(Crag::CragNode n, const CragVolume& volume) {

	const util::point<float, 3> resolution = volume.getResolution();
	const util::point<float, 3> offset     = volume.getOffset();

	util::point<int, 3> sampleRadius(
			std::max(1, (int)(_sampleRadius/resolution.x())),
			std::max(1, (int)(_sampleRadius/resolution.y())),
			std::max(1, (int)(_sampleRadius/resolution.z())));
	util::point<int, 3> sampleDensity(
			std::max(1, (int)(_sampleDensity/resolution.x())),
			std::max(1, (int)(_sampleDensity/resolution.y())),
			std::max(1, (int)(_sampleDensity/resolution.z())));

	// for each boundary point x
	for (int z = 0; z < volume.getDiscreteBoundingBox().depth();  z++)
	for (int y = 0; y < volume.getDiscreteBoundingBox().height(); y++)
	for (int x = 0; x < volume.getDiscreteBoundingBox().width();  x++) {

		// point foreground?
		if (volume.data()(x, y, z) == 0)
			continue;

		// at volume boundary?
		if (x == 0 || y == 0 || z == 0 ||
		    x == volume.getDiscreteBoundingBox().width()  - 1 ||
		    y == volume.getDiscreteBoundingBox().height() - 1 ||
		    z == volume.getDiscreteBoundingBox().depth()  - 1) {

			// good to go

		// has background neighbor?
		} else if (volume.data()(x-1, y, z) == 0 ||
		           volume.data()(x+1, y, z) == 0 ||
		           volume.data()(x, y-1, z) == 0 ||
		           volume.data()(x, y+1, z) == 0 ||
		           volume.data()(x, y, z-1) == 0 ||
		           volume.data()(x, y, z+1) == 0) {

			// good to go

		// otherwise
		} else {

			continue;
		}

		util::point<float, 3> a;
		int numSamples = 0;

		// sample points in local neighborhood
		for (int dz = -sampleRadius.z(); dz <= sampleRadius.z(); dz += sampleDensity.z())
		for (int dy = -sampleRadius.y(); dy <= sampleRadius.y(); dy += sampleDensity.y())
		for (int dx = -sampleRadius.x(); dx <= sampleRadius.x(); dx += sampleDensity.x()) {

			if (x + dx < 0 || y + dy < 0 || z + dz < 0 ||
			    x + dx >= volume.getDiscreteBoundingBox().width() ||
			    y + dy >= volume.getDiscreteBoundingBox().height() ||
			    z + dz >= volume.getDiscreteBoundingBox().depth())
				continue;

			if (volume(x + dx, y + dy, z + dz)) {

				a += util::point<float,3>(x + dx, y + dy, z + dz);
				numSamples++;
			}
		}

		// compute average a of coordinates inside the volume
		//   direction of ray is a -> x
		a = a/numSamples;

		// transform a and x into units of volume
		a = offset + a*resolution;
		util::point<float, 3> b = offset + util::point<float, 3>(x, y, z)*resolution;

		// create ray
		util::ray<float, 3> ray(b, (b - a)/length(b - a));

		// walk backwards on ray until we leave the volume
		util::point<float, 3> c(x, y, z);
		float distance = 0;
		while (c.x() >= 0 && c.y() >= 0 && c.z() >= 0 &&
		       c.x() < volume.getDiscreteBoundingBox().width() &&
		       c.y() < volume.getDiscreteBoundingBox().height() &&
		       c.z() < volume.getDiscreteBoundingBox().depth() &&
			   volume((int)c.x(), (int)c.y(), (int)c.z())) {

			c -= ray.direction()/resolution;
			distance += 1.0;
		}

		// travelled distance should be length of ray
		ray.direction() *= distance;

		(*this)[n].push_back(ray);
	}
}

