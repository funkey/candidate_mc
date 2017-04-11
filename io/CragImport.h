#ifndef CANDIDATE_MC_IO_CRAG_IMPORT_H__
#define CANDIDATE_MC_IO_CRAG_IMPORT_H__

#include <map>
#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <io/Hdf5VolumeReader.h>
#include <inference/Costs.h>
#include "volumes.h"

template <typename T>
void
readVolumeFromOption(ExplicitVolume<T>& volume, std::string option) {

	// hdf file given?
	size_t sepPos = option.find_first_of(":");
	if (sepPos != std::string::npos) {

		std::string hdfFileName = option.substr(0, sepPos);
		std::string dataset     = option.substr(sepPos + 1);

		vigra::HDF5File file(hdfFileName, vigra::HDF5File::OpenMode::ReadOnly);
		Hdf5VolumeReader hdfReader(file);
		hdfReader.readVolume(volume, dataset);

	// read volume from set of images
	} else {

		std::vector<std::string> files = getImageFiles(option);
		volume = readVolume<T>(files);
	}
}

class CragImport {

public:

	/**
	 * Import a CRAG from a merge tree image.
	 *
	 * @param mergetree
	 *              Path to a merge tree image. A merge tree image has twice the 
	 *              resolution as the original image to delineate edges between 
	 *              voxels. Successive thresholding of the merge tree images 
	 *              reveals all candidates on each level of the merge tree.
	 * @param crag
	 *              The CRAG to fill.
	 * @param volumes
	 *              A node map for the leaf node volmes.
	 * @param resolution
	 *              The (original) resolution of the volume, to be stored in the 
	 *              volumes.
	 * @param offset
	 *              The (original) offset of the volume, to be stored in the 
	 *              volumes.
	 */
	void readCrag(
			std::string           mergetree,
			Crag&                 crag,
			CragVolumes&          volumes,
			util::point<float, 3> resolution,
			util::point<float, 3> offset);

	/**
	 * Import a CRAG from a supervoxel image or volume and a merge history.
	 *
	 * @param supervoxel
	 *              Path to a supervoxel image or directory of images for 
	 *              volumes. In the supervoxel volume, each voxel is labelled 
	 *              with a unique supervoxel id.
	 * @param mergeHistory
	 *              Path to a text file containing a merge history as rows of 
	 *              the form "a b c", stating that candidate a got merged with b 
	 *              into new candidate c. Supervoxels from the supervoxel volume 
	 *              are the initial candidates.
	 * @param crag
	 *              The CRAG to fill.
	 * @param volumes
	 *              A node map for the leaf node volmes.
	 * @param resolution
	 *              The resolution of the volume, to be stored in the volumes.
	 * @param offset
	 *              The offset of the volume, to be stored in the volumes.
	 * @param mergeCosts
	 *              The merge-score of each candidate, if it was stored in the
	 *              merge history file.
	 * @param idToNode
	 *              Map the id on merge history with the node id on crag
	 */
	void readCragFromMergeHistory(
			std::string                supervoxels,
			std::string                mergeHistory,
			Crag&                      crag,
			CragVolumes&               volumes,
			util::point<float, 3>      resolution,
			util::point<float, 3>      offset,
			Costs&                     mergeCosts,
			Crag::NodeMap<int>&        nodeToId);

	/**
	 * Import a CRAG of depth 1 from a supervoxel image or volume and a 
	 * segmentation image or volume.
	 *
	 * @param supervoxel
	 *              Path to a supervoxel image or directory of images for 
	 *              volumes. In the supervoxel volume, each voxel is labelled 
	 *              with a unique supervoxel id.
	 * @param candidateSegmentation
	 *              A labelled volume representing a segmentation. The final 
	 *              CRAG will have one candidate per supervoxel, and larger 
	 *              candidates per segment. Supervoxels will be assigned to the 
	 *              segment with which they have largest overlap.
	 * @param crag
	 *              The CRAG to fill.
	 * @param volumes
	 *              A node map for the leaf node volmes.
	 * @param resolution
	 *              The resolution of the volume, to be stored in the volumes.
	 * @param offset
	 *              The offset of the volume, to be stored in the volumes.
	 */
	void readCragFromCandidateSegmentation(
			std::string           supervoxels,
			std::string           candidateSegmentation,
			Crag&                 crag,
			CragVolumes&          volumes,
			util::point<float, 3> resolution,
			util::point<float, 3> offset);

	/**
	 * Read a flat CRAG from a volume of supervoxels.
	 *
	 * @param supervoxels
	 *              A supervoxel volumes. In the supervoxel volume, each voxel 
	 *              is labelled with a unique supervoxel id.
	 * @param crag
	 *              The CRAG to fill.
	 * @param volumes
	 *              A node map for the leaf node volmes.
	 * @param resolution
	 *              The resolution of the volume, to be stored in the volumes.
	 * @param offset
	 *              The offset of the volume, to be stored in the volumes.
	 */
	std::map<int, Crag::Node> readSupervoxels(
			const ExplicitVolume<int>& supervoxels,
			Crag&                      crag,
			CragVolumes&               volumes,
			util::point<float, 3>      resolution,
			util::point<float, 3>      offset);
};

#endif // CANDIDATE_MC_IO_CRAG_IMPORT_H__

