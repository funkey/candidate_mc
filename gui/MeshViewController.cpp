#include "MeshViewController.h"
#include <sg_gui/MarchingCubes.h>
#include <util/ProgramOptions.h>

util::ProgramOption optionCubeSize(
		util::_long_name        = "cubeSize",
		util::_description_text = "The size of a cube for the marching cubes visualization.",
		util::_default_value    = 10);

MeshViewController::MeshViewController(
		const Crag& crag,
		std::shared_ptr<ExplicitVolume<float>> labels) :
	_crag(crag),
	_labels(labels),
	_meshes(std::make_shared<sg_gui::Meshes>()) {}

void
MeshViewController::loadMeshes(const std::vector<Crag::Node>& nodes) {

	for (Crag::Node node : nodes)
		addMesh(node);

	send<sg_gui::SetMeshes>(_meshes);
}

void
MeshViewController::onSignal(sg_gui::VolumePointSelected& signal) {

	unsigned int x, y, z;
	_labels->getDiscreteCoordinates(
			signal.position().x(),
			signal.position().y(),
			signal.position().z(),
			x, y, z);
	Crag::Node n = _crag.nodeFromId((*_labels)(x, y, z));

	if (n == lemon::INVALID)
		return;

	if (_meshes->contains(_crag.id(n)))
		removeMesh(n);
	else
		addMesh(n);

	send<sg_gui::SetMeshes>(_meshes);
}

void
MeshViewController::addMesh(Crag::Node n) {

	const ExplicitVolume<unsigned char>& volume = _crag.getVolume(n);

	typedef ExplicitVolumeAdaptor<ExplicitVolume<unsigned char>> Adaptor;
	Adaptor adaptor(volume);

	sg_gui::MarchingCubes<Adaptor> marchingCubes;
	std::shared_ptr<sg_gui::Mesh> mesh = marchingCubes.generateSurface(
			adaptor,
			sg_gui::MarchingCubes<Adaptor>::AcceptAbove(0.5),
			optionCubeSize,
			optionCubeSize,
			optionCubeSize);
	_meshes->add(_crag.id(n), mesh);
}

void
MeshViewController::removeMesh(Crag::Node n) {

	_meshes->remove(_crag.id(n));
}
