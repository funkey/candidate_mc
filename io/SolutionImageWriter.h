#ifndef CANDIDATE_MC_IO_SOLUTION_IMAGE_WRITER_H__
#define CANDIDATE_MC_IO_SOLUTION_IMAGE_WRITER_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <inference/CragSolution.h>

class SolutionImageWriter {

public:

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
};

#endif // CANDIDATE_MC_IO_SOLUTION_IMAGE_WRITER_H__

