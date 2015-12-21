#ifndef CANDIDATE_MC_IO_SOLUTION_IMAGE_WRITER_H__
#define CANDIDATE_MC_IO_SOLUTION_IMAGE_WRITER_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <inference/CragSolution.h>

class SolutionImageWriter {

public:

	/**
	 * Set a region of interest to be exported. This region can be larger than 
	 * the bounding box of all volumes, and in particular can be the bounding 
	 * box of the intensity volume to create an image of the same size with the 
	 * candidate volumes properly located in it.
	 *
	 * If not set, the bounding box of the volumes is used (which might be 
	 * smaller than the bounding box of the intensity volume).
	 */
	void setExportArea(const util::box<float, 3>& bb);

	/**
	 * Store the solution as label image in the given image file.
	 */
	void write(
			const Crag& crag,
			const CragVolumes& volumes,
			const CragSolution& solution,
			const std::string& filename,
			bool drawBoundary = false);

private:

	typedef vigra::TinyVector<unsigned int, 3> TinyVector3UInt;

	void drawBoundary(
			const CragVolumes&           volumes,
			Crag::Node                   n,
			vigra::MultiArray<3, float>& components,
			float                        value);

	util::box<float, 3> _volumesBB;
};

#endif // CANDIDATE_MC_IO_SOLUTION_IMAGE_WRITER_H__

