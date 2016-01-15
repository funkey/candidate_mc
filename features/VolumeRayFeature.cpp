#include "VolumeRayFeature.h"
#include <util/geometry.hpp>

double
VolumeRayFeature::maxVolumeRayPiercingDepth(Crag::CragNode u, Crag::CragNode v, util::ray<float, 3>& maxPiercingRay) {

	//bool debug = false;
	//if (_rays.getCrag().id(u) == 142 && _rays.getCrag().id(v) == 144)
		//debug = true;

	const CragVolume& volume = *_volumes[v];
	const util::point<float, 3> resolution = volume.getResolution();
	const util::point<float, 3> offset     = volume.getOffset();

	//if (debug) { std::cout  << "res: " << resolution << std::endl; }
	//if (debug) { std::cout  << "off: " << offset << std::endl; }

	double maxDistance = 0;

	for (auto ray : _rays[u]) {

		// position in world coordinates
		util::point<float, 3> start = ray.position();

		//if (debug) { std::cout  << "start in world coordinates: " << start << std::endl; }

		// transform to volume v coordinates
		start = (start - offset)/resolution;

		//if (debug) { std::cout  << "start in target volume: " << start << std::endl; }

		// create a world-space unit direction vector, and transform it into v's 
		// volume space
		double rayLength = length(ray.direction());
		util::point<float, 3> direction = (ray.direction()/rayLength)/resolution;

		//if (debug) { std::cout  << "direction: " << direction << std::endl; }
		//if (debug) { std::cout  << "source bb: " << volume.getBoundingBox() << std::endl; }
		//if (debug) { std::cout  << "target bb: " << _volumes[u]->getBoundingBox() << std::endl; }
		//if (debug) { std::cout  << "target dbb: " << volume.getDiscreteBoundingBox() << std::endl; }

		// walk in u's ray direction until we enter the volume of v
		util::point<float, 3> x = start;
		double enter = 0;
		while (enter <= rayLength) {

			// entered v's volume?
			if (x.x() >= 0 && x.y() >= 0 && x.z() >= 0 &&
			    x.x() < volume.getDiscreteBoundingBox().width() &&
			    x.y() < volume.getDiscreteBoundingBox().height() &&
			    x.z() < volume.getDiscreteBoundingBox().depth() &&
			    volume((int)x.x(), (int)x.y(), (int)x.z()))
				break;

			x += direction;
			enter += 1.0;

			//if (debug) { std::cout  << "next x: " << x << std::endl; }
		}

		//if (debug) { std::cout  << "entered v's volume at " << x << std::endl; }

		// walk in u's ray direction until we leave the volume of v
		double leave = enter;
		while (leave <= rayLength) {

			// still inside?
			if (x.x() >= 0 && x.y() >= 0 && x.z() >= 0 &&
			    x.x() < volume.getDiscreteBoundingBox().width() &&
			    x.y() < volume.getDiscreteBoundingBox().height() &&
			    x.z() < volume.getDiscreteBoundingBox().depth() &&
			    volume((int)x.x(), (int)x.y(), (int)x.z())) {

				x += direction;
				leave += 1.0;

				//if (debug) { std::cout  << "next x: " << x << std::endl; }

			} else {

				break;
			}
		}

		//if (debug && leave > enter) { std::cout << "left at " << leave << std::endl; }

		double distance = leave - enter;
		if (distance >= maxDistance) {

			maxDistance = distance;
			maxPiercingRay = ray;
		}

		//if (debug && leave > enter) { std::cout << "piercing depth is " << distance << std::endl; }
	}

	//if (debug) { std::cout << "max piercing depth is " << maxDistance << std::endl; }

	return maxDistance;
}
