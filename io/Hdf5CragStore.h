#ifndef CANDIDATE_MC_IO_HDF_CRAG_STORE_H__
#define CANDIDATE_MC_IO_HDF_CRAG_STORE_H__

#include <vigra/hdf5impex.hxx>
#include "Hdf5GraphReader.h"
#include "Hdf5GraphWriter.h"
#include "Hdf5DigraphReader.h"
#include "Hdf5DigraphWriter.h"
#include "Hdf5VolumeReader.h"
#include "Hdf5VolumeWriter.h"
#include "CragStore.h"

class Hdf5CragStore :
		public CragStore,
		public Hdf5GraphReader,
		public Hdf5GraphWriter,
		public Hdf5DigraphReader,
		public Hdf5DigraphWriter,
		public Hdf5VolumeReader,
		public Hdf5VolumeWriter {

public:

	Hdf5CragStore(std::string projectFile) :
		Hdf5GraphReader(_hdfFile),
		Hdf5GraphWriter(_hdfFile),
		Hdf5DigraphReader(_hdfFile),
		Hdf5DigraphWriter(_hdfFile),
		Hdf5VolumeReader(_hdfFile),
		Hdf5VolumeWriter(_hdfFile),
		_hdfFile(
				projectFile,
				vigra::HDF5File::OpenMode::ReadWrite) {}


	/**
	 * Store a candidate region adjacency graph (CRAG).
	 */
	void saveCrag(const Crag& crag) override;

	/**
	 * Store features for the candidates (i.e., the nodes) of a CRAG.
	 */
	void saveNodeFeatures(const Crag& crag, const NodeFeatures& features) override;

	/**
	 * Store features for adjacent candidates (i.e., the edges) of a CRAG.
	 */
	void saveEdgeFeatures(const Crag& crag, const EdgeFeatures& features) override;

	/**
	 * Store the min and max values of the node features.
	 */
	void saveNodeFeaturesMinMax(
			const std::vector<double>& min,
			const std::vector<double>& max) override;

	/**
	 * Store the min and max values of the edge features.
	 */
	void saveEdgeFeaturesMinMax(
			const std::vector<double>& min,
			const std::vector<double>& max) override;

	/**
	 * Retrieve the candidate region adjacency graph (CRAG) associated to this 
	 * store.
	 */
	void retrieveCrag(Crag& crag) override;

	/**
	 * Retrieve features for the candidates (i.e., the nodes) of the CRAG 
	 * associated to this store.
	 */
	void retrieveNodeFeatures(const Crag& crag, NodeFeatures& features) override;

	/**
	 * Retrieve features for adjacent candidates (i.e., the edges) of the CRAG 
	 * associated to this store.
	 */
	void retrieveEdgeFeatures(const Crag& crag, EdgeFeatures& features) override;

	/**
	 * Retrieve the min and max values of the node features.
	 */
	void retrieveNodeFeaturesMinMax(
			std::vector<double>& min,
			std::vector<double>& max) override;

	/**
	 * Retrieve the min and max values of the edge features.
	 */
	void retrieveEdgeFeaturesMinMax(
			std::vector<double>& min,
			std::vector<double>& max) override;

	/**
	 * Store a segmentation, represented by sets of leaf nodes.
	 */
	void saveSegmentation(
			const Crag&                              crag,
			const std::vector<std::set<Crag::Node>>& segmentation,
			std::string                              name) override;

	/**
	 * Retrieve a segmentation, represented by sets of leaf nodes.
	 */
	void retrieveSegmentation(
			const Crag&                        crag,
			std::vector<std::set<Crag::Node>>& segmentation,
			std::string                        name) override;

	/**
	 * Get a list of the names of all stored segmentations.
	 */
	std::vector<std::string> getSegmentationNames() override;

private:

	vigra::HDF5File _hdfFile;
};

#endif // CANDIDATE_MC_IO_HDF_CRAG_STORE_H__

