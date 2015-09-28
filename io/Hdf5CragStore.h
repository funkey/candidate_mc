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
	 * Save CRAG volumes. This will only store the volumes of leaf nodes, others 
	 * can be assembled from them.
	 */
	void saveVolumes(const CragVolumes& volumes) override;

	/**
	 * Store features for the candidates (i.e., the nodes) of a CRAG.
	 */
	void saveNodeFeatures(const Crag& crag, const NodeFeatures& features) override;

	/**
	 * Store features for adjacent candidates (i.e., the edges) of a CRAG.
	 */
	void saveEdgeFeatures(const Crag& crag, const EdgeFeatures& features) override;

	/**
	 * Store the skeletons for candidates of a CRAG.
	 */
	void saveSkeletons(const Crag& crag, const Skeletons& skeletons);

	/**
	 * Store a set of feature weights.
	 */
	void saveFeatureWeights(const FeatureWeights& weights) override;

	/**
	 * Store the min and max values of the node features.
	 */
	void saveFeaturesMin(const FeatureWeights& min);
	void saveFeaturesMax(const FeatureWeights& max);

	/**
	 * Retrieve the candidate region adjacency graph (CRAG) associated to this 
	 * store.
	 */
	void retrieveCrag(Crag& crag) override;

	/**
	 * Retrieve the volumes of CRAG candidates. For that, only the leaf node 
	 * volumes of the given CragVolumes are set, other volumes will later be 
	 * created on demand.
	 */
	void retrieveVolumes(CragVolumes& volumes) override;

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
	void retrieveFeaturesMin(FeatureWeights& min);
	void retrieveFeaturesMax(FeatureWeights& max);

	/**
	 * Retrieve skeletons for the candidates of the CRAG.
	 */
	void retrieveSkeletons(const Crag& crag, Skeletons& skeletons) override;

	/**
	 * Retrieve feature weights.
	 */
	void retrieveFeatureWeights(FeatureWeights& weights) override;

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

	/**
	 * Converts Position objects into array-like objects for HDF5 storage.
	 */
	struct PositionConverter {

		typedef float    ArrayValueType;
		static const int ArraySize = 3;

		vigra::ArrayVector<float> operator()(const Skeleton::Position& pos) const {

			vigra::ArrayVector<float> array(3);
			for (int i = 0; i < 3; i++)
				array[i] = pos[i];
			return array;
		}

		Skeleton::Position operator()(const vigra::ArrayVectorView<float>& array) const {

			Skeleton::Position pos;
			for (int i = 0; i < 3; i++)
				pos[i] = array[i];

			return pos;
		}

	};

	void writeGraphVolume(const GraphVolume& graphVolume);
	void readGraphVolume(GraphVolume& graphVolume);

	void writeWeights(const FeatureWeights& weights, std::string name);
	void readWeights(FeatureWeights& weights, std::string name);

	vigra::HDF5File _hdfFile;
};

#endif // CANDIDATE_MC_IO_HDF_CRAG_STORE_H__

