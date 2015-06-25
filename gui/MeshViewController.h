#ifndef CANDIDATE_MC_GUI_MESH_VIEW_CONTROLLER_H__
#define CANDIDATE_MC_GUI_MESH_VIEW_CONTROLLER_H__

#include <crag/Crag.h>
#include <scopegraph/Agent.h>
#include <sg_gui/VolumeView.h>
#include <sg_gui/MeshView.h>
#include <sg_gui/Meshes.h>

template <typename EV>
class ExplicitVolumeAdaptor {

public:

	typedef typename EV::value_type value_type;

	ExplicitVolumeAdaptor(const EV& ev) :
		_ev(ev) {}

	const util::box<float,3>& getBoundingBox() const { return _ev.getBoundingBox(); }

	float operator()(float x, float y, float z) const {

		if (!getBoundingBox().contains(x, y, z))
			return 0;

		unsigned int dx, dy, dz;

		_ev.getDiscreteCoordinates(x, y, z, dx, dy, dz);

		return _ev(dx, dy, dz);
	}

private:

	const EV& _ev;

};

class MeshViewController :
		public sg::Agent<
				MeshViewController,
				sg::Accepts<
						sg_gui::VolumePointSelected,
						sg_gui::MouseDown
				>,
				sg::Provides<
						sg_gui::SetMeshes
				>
		> {

public:

	MeshViewController(
			const Crag&                            crag,
			std::shared_ptr<ExplicitVolume<float>> labels);

	void loadMeshes(const std::vector<Crag::Node>& nodes);

	void onSignal(sg_gui::VolumePointSelected& signal);

	void onSignal(sg_gui::MouseDown& signal);

private:

	void nextVolume();

	void prevVolume();

	void loadMesh(Crag::Node n);

	void addMesh(Crag::Node n);

	void removeMesh(Crag::Node n);

	const Crag& _crag;

	std::shared_ptr<ExplicitVolume<float>> _labels;

	std::shared_ptr<sg_gui::Meshes> _meshes;

	std::map<Crag::Node, std::shared_ptr<sg_gui::Mesh>> _meshCache;

	Crag::Node _current;

	std::vector<Crag::Node> _path;
};

#endif // CANDIDATE_MC_GUI_MESH_VIEW_CONTROLLER_H__

