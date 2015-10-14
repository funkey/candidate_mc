#include <boost/filesystem.hpp>
#include <vigra/multi_impex.hxx>
#include <vigra/functorexpression.hxx>
#include "SolutionImageWriter.h"

void
SolutionImageWriter::write(
		const Crag& crag,
		const CragVolumes& volumes,
		const CragSolution& solution,
		const std::string& basename,
		bool boundary) {

	if (_volumesBB.isZero())
		_volumesBB = volumes.getBoundingBox();
	util::point<float, 3> resolution;
	for (Crag::CragNode n : crag.nodes()) {

		if (!crag.isLeafNode(n))
			continue;
		resolution = volumes[n]->getResolution();
		break;
	}

	// create a vigra multi-array large enough to hold all volumes
	vigra::MultiArray<3, float> components(
			vigra::Shape3(
				_volumesBB.width() /resolution.x(),
				_volumesBB.height()/resolution.y(),
				_volumesBB.depth() /resolution.z()),
			std::numeric_limits<int>::max());

	// background for areas without candidates
	if (boundary)
		components = 0.25;
	else
		components = 0;

	for (Crag::CragNode n : crag.nodes()) {

		// draw only selected nodes
		if (!solution.selected(n))
			continue;

		const util::point<float, 3>&      volumeOffset     = volumes[n]->getOffset();
		const util::box<unsigned int, 3>& volumeDiscreteBB = volumes[n]->getDiscreteBoundingBox();

		util::point<unsigned int, 3> begin = (volumeOffset - _volumesBB.min())/resolution;
		util::point<unsigned int, 3> end   = begin +
				util::point<unsigned int, 3>(
						volumeDiscreteBB.width(),
						volumeDiscreteBB.height(),
						volumeDiscreteBB.depth());

		// fill id of connected component
		vigra::combineTwoMultiArrays(
			volumes[n]->data(),
			components.subarray(TinyVector3UInt(&begin[0]),TinyVector3UInt(&end[0])),
			components.subarray(TinyVector3UInt(&begin[0]),TinyVector3UInt(&end[0])),
			vigra::functor::ifThenElse(
					vigra::functor::Arg1() == vigra::functor::Param(1),
					vigra::functor::Param(solution.label(n)),
					vigra::functor::Arg2()
		));
	}

	if (boundary) {

		// gray boundary for all leaf nodes
		for (Crag::CragNode n : crag.nodes())
			if (crag.isLeafNode(n))
				drawBoundary(volumes, n, components, 0.5);

		// black boundary for all selected nodes
		for (Crag::CragNode n : crag.nodes())
			if (solution.selected(n))
				drawBoundary(volumes, n, components, 0);
	}

	if (components.shape(2) > 1) {

		boost::filesystem::create_directory(basename);
		for (unsigned int z = 0; z < components.shape(2); z++) {

			std::stringstream ss;
			ss << std::setw(4) << std::setfill('0') << z;
			vigra::exportImage(
					components.bind<2>(z),
					vigra::ImageExportInfo((basename + "/" + ss.str() + ".tif").c_str()));
		}

	} else {

		vigra::exportImage(
				components.bind<2>(0),
				vigra::ImageExportInfo((basename + ".tif").c_str()));
	}
}

void
SolutionImageWriter::drawBoundary(
		const CragVolumes&           volumes,
		Crag::Node                   n,
		vigra::MultiArray<3, float>& components,
		float                        value) {

	const CragVolume& volume = *volumes[n];
	const util::box<unsigned int, 3>& volumeDiscreteBB = volume.getDiscreteBoundingBox();
	const util::point<float, 3>&      volumeOffset     = volume.getOffset();
	util::point<unsigned int, 3>      begin            = (volumeOffset - _volumesBB.min())/volume.getResolution();

	bool hasZ = (volumeDiscreteBB.depth() > 1);

	// draw boundary
	for (unsigned int z = 0; z < volumeDiscreteBB.depth();  z++)
	for (unsigned int y = 0; y < volumeDiscreteBB.height(); y++)
	for (unsigned int x = 0; x < volumeDiscreteBB.width();  x++) {

		// only inside voxels
		if (!volume.data()(x, y, z))
			continue;

		if ((hasZ && (z == 0 || z == volumeDiscreteBB.depth()  - 1)) ||
			y == 0 || y == volumeDiscreteBB.height() - 1 ||
			x == 0 || x == volumeDiscreteBB.width()  - 1) {

			components(
					begin.x() + x,
					begin.y() + y,
					begin.z() + z) = value;
			continue;
		}

		bool done = false;
		for (int dz = (hasZ ? -1 : 0); dz <= (hasZ ? 1 : 0) && !done; dz++)
		for (int dy = -1; dy <= 1 && !done; dy++)
		for (int dx = -1; dx <= 1 && !done; dx++) {

			if (std::abs(dx) + std::abs(dy) + std::abs(dz) > 1)
				continue;

			if (!volume.data()(x + dx, y + dy, z + dz)) {

				components(
						begin.x() + x,
						begin.y() + y,
						begin.z() + z) = value;
				done = true;
			}
		}
	}
}
