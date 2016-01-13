#ifndef CANDIDATE_MC_FEATURES_VOLUME_RAYS_H__
#define CANDIDATE_MC_FEATURES_VOLUME_RAYS_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <util/ray.hpp>

/**
 * A set of rays leaving the volume surface of a node. The length of the 
 * direction vector encodes the elongation of the node in this direction.
 */
class VolumeRays : public Crag::NodeMap<std::vector<util::ray<float,3>>>, public Volume {

public:

	/**
	 * @param crag The crag to extract rays for.
	 */
	VolumeRays(const Crag& crag) :
			Crag::NodeMap<std::vector<util::ray<float,3>>>(crag),
			_crag(crag),
			_sampleRadius(10),
			_sampleDensity(2) {}

	/**
	 *
	 * @param sampleRadius
	 *             The size of the sphere to use to estimate the surface normal 
	 *             of boundary points.
	 *
	 * @param sampleDensity
	 *             Distance between sample points in the normal estimation 
	 *             sphere.
	 */
	void extractFromVolumes(const CragVolumes& volumes, float sampleRadius, float sampleDensity);

	const Crag& getCrag() const { return _crag; }

private:

	void extract(Crag::CragNode n, const CragVolume& volume);

	util::box<float,3> computeBoundingBox() const {

		util::box<float,3> boundingBox;

		for (Crag::CragNode n : _crag.nodes())
			for (auto ray : (*this)[n]) {

				boundingBox.fit(ray.position());
				boundingBox.fit(ray.position() + ray.direction());
			}

		return boundingBox;
	}

	const Crag& _crag;

	// size of spherical region to take samples for normal estimation
	float _sampleRadius;

	// distance between samples
	float _sampleDensity;
};

#endif // CANDIDATE_MC_FEATURES_VOLUME_RAYS_H__

