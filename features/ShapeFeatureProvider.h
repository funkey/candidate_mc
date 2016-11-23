#ifndef CANDIDATE_MC_FEATURES_SHAPE_FEATURE_PROVIDER_H__
#define CANDIDATE_MC_FEATURES_SHAPE_FEATURE_PROVIDER_H__

#include <region_features/RegionFeatures.h>

#include "FeatureProvider.h"

/**
 * Computes shape features for nodes.
 */
class ShapeFeatureProvider : public FeatureProvider<ShapeFeatureProvider> {

public:

	struct Parameters {

		Parameters() :
			numAnglePoints(50),
			contourVecAsArcSegmentRatio(0.1),
			numAngleHistBins(16) {}

		/**
		 * The number of points to sample equidistantly on the contour of nodes.
		 */
		int numAnglePoints;

		/**
		 * The amount to walk on the contour from a sample point in either 
		 * direction, to estimate the angle. Values are between 0 (at the sample 
		 * point) and 1 (at the next sample point).
		 */
		double contourVecAsArcSegmentRatio;

		/**
		 * The number of histogram bins for the measured angles.
		 */
		int numAngleHistBins;
	};

	ShapeFeatureProvider(
			const Crag& crag,
			const CragVolumes& volumes,
			const Parameters& parameters = Parameters()) :
		_crag(crag),
		_volumes(volumes),
		_parameters(parameters) {

			RegionFeatures<2, float, unsigned char>::Parameters parameters2d;
			parameters2d.computeStatistics    = false;
			parameters2d.computeShapeFeatures = true;
			parameters2d.shapeFeaturesParameters.numAnglePoints              = _parameters.numAnglePoints;
			parameters2d.shapeFeaturesParameters.contourVecAsArcSegmentRatio = _parameters.contourVecAsArcSegmentRatio;
			parameters2d.shapeFeaturesParameters.numAngleHistBins            = _parameters.numAngleHistBins;

			_2dRegionFeatures = RegionFeatures<2, float, unsigned char>(parameters2d);

			RegionFeatures<3, float, unsigned char>::Parameters parameters3d;
			parameters3d.computeStatistics    = false;
			parameters3d.computeShapeFeatures = true;
			parameters3d.shapeFeaturesParameters.numAnglePoints              = _parameters.numAnglePoints;
			parameters3d.shapeFeaturesParameters.contourVecAsArcSegmentRatio = _parameters.contourVecAsArcSegmentRatio;
			parameters3d.shapeFeaturesParameters.numAngleHistBins            = _parameters.numAngleHistBins;

			_3dRegionFeatures = RegionFeatures<3, float, unsigned char>(parameters3d);
		}

	template <typename ContainerT>
	void appendNodeFeatures(const Crag::CragNode n, ContainerT& adaptor) {

		// the "label" image
		const vigra::MultiArray<3, unsigned char>& labelImage = _volumes[n]->data();

		if (_crag.type(n) == Crag::SliceNode)
			_2dRegionFeatures.fill(labelImage.bind<2>(0), adaptor);
		else
			_3dRegionFeatures.fill(labelImage, adaptor);
	}

	std::map<Crag::NodeType, std::vector<std::string>> getNodeFeatureNames() const override {

		std::map<Crag::NodeType, std::vector<std::string>> names;

		names[Crag::SliceNode]      = _2dRegionFeatures.getFeatureNames();
		names[Crag::VolumeNode]     = _3dRegionFeatures.getFeatureNames();
		names[Crag::AssignmentNode] = _3dRegionFeatures.getFeatureNames();

		return names;
	}

private:

	const Crag&        _crag;
	const CragVolumes& _volumes;

	Parameters _parameters;

	RegionFeatures<2, float, unsigned char> _2dRegionFeatures;
	RegionFeatures<3, float, unsigned char> _3dRegionFeatures;
};

#endif // CANDIDATE_MC_FEATURES_SHAPE_FEATURE_PROVIDER_H__

