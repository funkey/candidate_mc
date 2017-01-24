#include "ContactFeature.h"
#include <util/Logger.h>
#include <util/helpers.hpp>

logger::LogChannel contactfeaturelog("contactfeaturelog", "[ContactFeature] ");

std::vector<double>
ContactFeature::compute(Crag::CragEdge e) {

	std::vector<double> features(_thresholds.size()*4*2 + 4 + _thresholds.size());

	LOG_ALL(contactfeaturelog) << "computing contact feature for thresholds " << _thresholds << std::endl;

	// number of voxels brighter than thresholds in contact, plus size
	//
	// initialize threshold counts with 1 for numerical stability (and because 
	// this is how it's done in Gala)
	std::vector<int> contactCounts(_thresholds.size() + 1, 1);
	(*contactCounts.rbegin()) = 0;

	// get all unique voxels adjacent to contact
	std::set<vigra::GridGraph<3>::Node> contactVoxels;
	const auto& gridGraph = _crag.getGridGraph();
	for (Crag::CragEdge leafEdge : _crag.leafEdges(e))
		for (vigra::GridGraph<3>::Edge ae : _crag.getAffiliatedEdges(leafEdge)) {

			contactVoxels.insert(gridGraph.u(ae));
			contactVoxels.insert(gridGraph.v(ae));
		}

	// count voxels above thresholds
	for (auto n : contactVoxels) {

		float value = _boundaries[n];

		for (int i = 0; i < _thresholds.size(); i++)
			if (value > _thresholds[i])
				contactCounts[i]++;

		(*contactCounts.rbegin())++;
	}

	// count voxels inside candidates
	std::vector<int> uCounts = countVoxels(e.u());
	std::vector<int> vCounts = countVoxels(e.v());

	double uVolRatio = (double)(*contactCounts.rbegin())/(*uCounts.rbegin());
	double vVolRatio = (double)(*contactCounts.rbegin())/(*vCounts.rbegin());

	// compute "contact matrix"
	std::vector<double> contactScores;
	for (int i = 0; i < _thresholds.size(); i++) {

		// for u
		double contactRatio = (double)contactCounts[i]/uCounts[i];
		double normalizedContactRatio = contactRatio/uVolRatio;
		contactScores.push_back(contactRatio);
		contactScores.push_back(normalizedContactRatio);

		// for v
		contactRatio = (double)contactCounts[i]/vCounts[i];
		normalizedContactRatio = contactRatio/vVolRatio;
		contactScores.push_back(contactRatio);
		contactScores.push_back(normalizedContactRatio);
	}

	LOG_ALL(contactfeaturelog) << "contanct scores of edge " << _crag.id(e.u()) << ", " << _crag.id(e.v()) << ": " << contactScores << std::endl;

	// copy flattened contact matrix and its log into feature vector
	for (int i = 0; i < contactScores.size(); i++) {

		features[i] = contactScores[i];
		features[contactScores.size() + i] = log(contactScores[i]);
	}

	// add the volume ratios as well
	features[2*contactScores.size()    ] = log(uVolRatio);
	features[2*contactScores.size() + 1] = log(vVolRatio);
	features[2*contactScores.size() + 2] = uVolRatio;
	features[2*contactScores.size() + 3] = vVolRatio;

	// finally, add the size of the contact
	for (int i = 0; i < _thresholds.size(); i++)
		features[2*contactScores.size() + 4 + i] = contactCounts[i];

	return features;
}

std::vector<int>
ContactFeature::countVoxels(Crag::CragNode n) {

	const CragVolume& volume = *_volumes[n];

	const util::box<float, 3>&   nodeBoundingBox    = volume.getBoundingBox();
	util::point<unsigned int, 3> nodeSize           = (nodeBoundingBox.max() - nodeBoundingBox.min())/volume.getResolution();
	util::point<float, 3>        nodeOffset         = nodeBoundingBox.min() - _boundaries.getBoundingBox().min();
	util::point<unsigned int, 3> nodeDiscreteOffset = nodeOffset/volume.getResolution();

	// a view to the boundary image for the node bounding box
	typedef vigra::MultiArrayView<3, float>::difference_type Shape;
	vigra::MultiArrayView<3, float> boundaryNodeImage =
			_boundaries.data().subarray(
					Shape(
							nodeDiscreteOffset.x(),
							nodeDiscreteOffset.y(),
							nodeDiscreteOffset.z()),
					Shape(
							nodeDiscreteOffset.x() + nodeSize.x(),
							nodeDiscreteOffset.y() + nodeSize.y(),
							nodeDiscreteOffset.z() + nodeSize.z()));

	// initialize all threshold counts with 1 for numerical stability (and also 
	// because this is how it's done in Gala)
	std::vector<int> counts = std::vector<int>(_thresholds.size() + 1, 1);
	(*counts.rbegin()) = 0;

	for (unsigned int z = 0; z < volume.depth(); z++)
	for (unsigned int y = 0; y < volume.height(); y++)
	for (unsigned int x = 0; x < volume.width(); x++) {

		if (volume(x, y, z) == 0)
			// not part of volume
			continue;

		float value = boundaryNodeImage(x, y, z);

		for (int i = 0; i < _thresholds.size(); i++)
			if (value > _thresholds[i])
				counts[i]++;

		(*counts.rbegin())++;
	}

	return counts;
}
