#ifndef CANDIDATE_MC_IO_CRAG_IMPORT_H__
#define CANDIDATE_MC_IO_CRAG_IMPORT_H__

#include <crag/Crag.h>
#include <crag/CragVolumes.h>

class CragImport {

public:

	/**
	 * Import a CRAG from a merge tree image.
	 *
	 * @param mergetree
	 *              Path to a merge tree image. A merge tree image has twice the 
	 *              resolution as the original image to delineate edges between 
	 *              pixels. Successive thresholding of the merge tree images 
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
	 * Import a CRAG from a superpixel image or volume and a merge history.
	 *
	 * @param superpixel
	 *              Path to a superpixel image or directory of images for 
	 *              volumes. In the superpixel volume, each pixel is labelled 
	 *              with a unique superpixel id.
	 * @param mergeHistory
	 *              Path to a text file containing a merge history as rows of 
	 *              the form "a b c", stating that candidate a got merged with b 
	 *              into new candidate c. Superpixels from the superpixel volume 
	 *              are the initial candidates.
	 * @param mergeScores
	 *              Path to a text file containing a score for each merge in the 
	 *              merge history, in the same order. Can be used to stop 
	 *              merging after a certain threshold.
	 * @param crag
	 *              The CRAG to fill.
	 * @param volumes
	 *              A node map for the leaf node volmes.
	 * @param resolution
	 *              The resolution of the volume, to be stored in the volumes.
	 * @param offset
	 *              The offset of the volume, to be stored in the volumes.
	 */
	void readCrag(
			std::string           superpixels,
			std::string           mergeHistory,
			std::string           mergeScores,
			Crag&                 crag,
			CragVolumes&          volumes,
			util::point<float, 3> resolution,
			util::point<float, 3> offset);

	/**
	 * Import a CRAG of depth 1 from a superpixel image or volume and a 
	 * segmentation image or volume.
	 *
	 * @param superpixel
	 *              Path to a superpixel image or directory of images for 
	 *              volumes. In the superpixel volume, each pixel is labelled 
	 *              with a unique superpixel id.
	 * @param candidateSegmentation
	 *              A labelled volume representing a segmentation. The final 
	 *              CRAG will have one candidate per superpixel, and larger 
	 *              candidates per segment. Superpixels will be assigned to the 
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
	void readCrag(
			std::string           superpixels,
			std::string           candidateSegmentation,
			Crag&                 crag,
			CragVolumes&          volumes,
			util::point<float, 3> resolution,
			util::point<float, 3> offset);

private:

	std::map<int, Crag::Node> readSupervoxels(
			Crag&                 crag,
			CragVolumes&          volumes,
			util::point<float, 3> resolution,
			util::point<float, 3> offset,
			std::string           supervoxels);
};

#endif // CANDIDATE_MC_IO_CRAG_IMPORT_H__

