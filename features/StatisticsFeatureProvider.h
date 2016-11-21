#ifndef CANDIDATE_MC_FEATURES_STATISTICS_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_STATISTICS_FEATURE_PROVIDER_H__

#include <region_features/RegionFeatures.h>
#include <vigra/flatmorphology.hxx>
#include <vigra/multi_morphology.hxx>

#include "FeatureProvider.h"

/**
 * Computes several statistics (mean, variance, ...) of candidate voxels over an 
 * array of values.
 */
class StatisticsFeatureProvider : public FeatureProvider<StatisticsFeatureProvider> {

public:

	struct Parameters {

		Parameters() :
			wholeVolume(true),
			boundaryVoxels(true),
			computeCoordinateStatistics(true) {}

		/**
		 * Compute statistics over the complete volume of the candidate.
		 */
		bool wholeVolume;

		/**
		 * Compute statistics over the boundary voxels of the candidate.
		 */
		bool boundaryVoxels;

		/**
		 * Compute mean, variance, etc. on coordinate values.
		 */
		bool computeCoordinateStatistics;
	};

	/**
	 * @param values
	 *              An array of values to compute statistics over.
	 *
	 * @pararm valuesName
	 *              An optional name for the values. Will pre prepended to 
	 *              feature names.
	 *
	 * @param volumes
	 *              Crag volumes
	 *
	 * @param parameters
	 */
	StatisticsFeatureProvider(
			const ExplicitVolume<float>& values,
			const Crag& crag,
			const CragVolumes& volumes,
			const std::string valuesName = "values",
			const Parameters& parameters = Parameters()) :
		_values(values),
		_valuesName(valuesName),
		_crag(crag),
		_volumes(volumes),
		_parameters(parameters) {

			RegionFeatures<2, float, unsigned char>::Parameters parameters2d;
			parameters2d.computeStatistics    = true;
			parameters2d.computeShapeFeatures = false;
			parameters2d.statisticsParameters.computeCoordinateStatistics = _parameters.computeCoordinateStatistics;

			_2dRegionFeatures = RegionFeatures<2, float, unsigned char>(parameters2d);

			RegionFeatures<3, float, unsigned char>::Parameters parameters3d;
			parameters3d.computeStatistics    = true;
			parameters3d.computeShapeFeatures = false;
			parameters3d.statisticsParameters.computeCoordinateStatistics = _parameters.computeCoordinateStatistics;

			_3dRegionFeatures = RegionFeatures<3, float, unsigned char>(parameters3d);
		}

	template <typename ContainerT>
	void appendNodeFeatures(const Crag::CragNode n, ContainerT& adaptor) {

		// the bounding box of the volume
		const util::box<float, 3>&   nodeBoundingBox    = _volumes[n]->getBoundingBox();
		util::point<unsigned int, 3> nodeSize           = (nodeBoundingBox.max() - nodeBoundingBox.min())/_volumes[n]->getResolution();
		util::point<float, 3>        nodeOffset         = nodeBoundingBox.min() - _values.getBoundingBox().min();
		util::point<unsigned int, 3> nodeDiscreteOffset = nodeOffset/_volumes[n]->getResolution();

		// a view to the values image for the node bounding box
		typedef vigra::MultiArrayView<3, float>::difference_type Shape;
		vigra::MultiArrayView<3, float> valuesNodeImage =
				_values.data().subarray(
						Shape(
								nodeDiscreteOffset.x(),
								nodeDiscreteOffset.y(),
								nodeDiscreteOffset.z()),
						Shape(
								nodeDiscreteOffset.x() + nodeSize.x(),
								nodeDiscreteOffset.y() + nodeSize.y(),
								nodeDiscreteOffset.z() + nodeSize.z()));

		// the "label" image
		const vigra::MultiArray<3, unsigned char>& labelImage = _volumes[n]->data();

		if (_parameters.wholeVolume) {

			if (_crag.type(n) == Crag::SliceNode)
				_2dRegionFeatures.fill(valuesNodeImage.bind<2>(0), labelImage.bind<2>(0), adaptor);
			else
				_3dRegionFeatures.fill(valuesNodeImage, labelImage, adaptor);
		}

		if (_parameters.boundaryVoxels) {

			vigra::MultiArray<3, unsigned char> boundaryImage = getBoundaryVoxelMask(labelImage);

			if (_crag.type(n) == Crag::SliceNode)
				_2dRegionFeatures.fill(valuesNodeImage.bind<2>(0), boundaryImage.bind<2>(0), adaptor);
			else
				_3dRegionFeatures.fill(valuesNodeImage, boundaryImage, adaptor);
		}
	}

	std::map<Crag::NodeType, std::vector<std::string>> getNodeFeatureNames() const override {

		std::map<Crag::NodeType, std::vector<std::string>> names;

		if (_parameters.wholeVolume) {

			names[Crag::SliceNode]      = _2dRegionFeatures.getFeatureNames(_valuesName);
			names[Crag::VolumeNode]     = _3dRegionFeatures.getFeatureNames(_valuesName);
			names[Crag::AssignmentNode] = _3dRegionFeatures.getFeatureNames(_valuesName);
		}

		if (_parameters.boundaryVoxels) {

			for (std::string name : _2dRegionFeatures.getFeatureNames(_valuesName + "boundary "))
				names[Crag::SliceNode].push_back(name);
			for (std::string name : _3dRegionFeatures.getFeatureNames(_valuesName + "boundary ")) {

				names[Crag::VolumeNode].push_back(name);
				names[Crag::AssignmentNode].push_back(name);
			}
		}

		return names;
	}

private:

	vigra::MultiArray<3, unsigned char> getBoundaryVoxelMask(const vigra::MultiArray<3, unsigned char>& labelImage) {

		unsigned int width  = labelImage.shape()[0];
		unsigned int height = labelImage.shape()[1];
		unsigned int depth  = labelImage.shape()[2];

		// create the boundary image by erosion and subtraction...
		vigra::MultiArray<3, unsigned char> erosionImage(labelImage.shape());
		if (depth == 1)
			vigra::discErosion(labelImage.bind<2>(0), erosionImage.bind<2>(0), 1);
		else
			vigra::multiBinaryErosion(labelImage, erosionImage, 1);
		vigra::MultiArray<3, unsigned char> boundaryImage(labelImage.shape());
		boundaryImage = labelImage;
		boundaryImage -= erosionImage;

		// ...and fix the border of the label image
		if (depth == 1) {

			for (unsigned int x = 0; x < width; x++) {

				boundaryImage(x, 0, 0)        |= labelImage(x, 0, 0);
				boundaryImage(x, height-1, 0) |= labelImage(x, height-1, 0);
			}

			for (unsigned int y = 1; y < height-1; y++) {

				boundaryImage(0, y, 0)       |= labelImage(0, y, 0);
				boundaryImage(width-1, y, 0) |= labelImage(width-1, y, 0);
			}

		} else {

			for (unsigned int z = 0; z < depth;  z++)
			for (unsigned int y = 0; y < height; y++)
			for (unsigned int x = 0; x < width;  x++)
				if (x == 0 || y == 0 || z == 0 || x == width - 1 || y == height -1 || z == depth - 1)
					boundaryImage(x, y, z) |= labelImage(x, y, z);
		}

		return boundaryImage;
	}

	const ExplicitVolume<float>& _values;

	std::string _valuesName;

	const Crag&        _crag;
	const CragVolumes& _volumes;

	Parameters _parameters;

	RegionFeatures<2, float, unsigned char> _2dRegionFeatures;
	RegionFeatures<3, float, unsigned char> _3dRegionFeatures;
};

#endif // CANDIDATE_MC_FEATURES_STATISTICS_FEATURE_PROVIDER_H__

