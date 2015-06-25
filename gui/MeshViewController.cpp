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
	_meshes(std::make_shared<sg_gui::Meshes>()),
	_current(lemon::INVALID) {}

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
		loadMesh(n);

	_current = n;
	_path.clear();

	send<sg_gui::SetMeshes>(_meshes);
}

void
MeshViewController::onSignal(sg_gui::MouseDown& signal) {

	if (signal.processed)
		return;

	if (signal.button == sg_gui::buttons::WheelUp && signal.modifiers & sg_gui::keys::ShiftDown)
		nextVolume();
	else if (signal.button == sg_gui::buttons::WheelDown && signal.modifiers & sg_gui::keys::ShiftDown)
		prevVolume();
	else
		return;

	signal.processed = true;
}

void
MeshViewController::nextVolume() {

	if (_current == lemon::INVALID)
		return;

	Crag::SubsetOutArcIt parentEdge(_crag, _crag.toSubset(_current));

	if (parentEdge == lemon::INVALID)
		return;

	Crag::Node parent = _crag.toRag(_crag.getSubsetGraph().oppositeNode(_crag.toSubset(_current), parentEdge));

	_path.push_back(_current);
	_current = parent;

	loadMesh(_current);

	send<sg_gui::SetMeshes>(_meshes);
}

void
MeshViewController::prevVolume() {

	if (_current == lemon::INVALID)
		return;

	if (_path.size() == 0)
		return;

	_current = _path.back();
	_path.pop_back();

	loadMesh(_current);

	send<sg_gui::SetMeshes>(_meshes);
}

void
MeshViewController::loadMesh(Crag::Node n) {

	_meshes->clear();
	addMesh(n);
}

void
MeshViewController::addMesh(Crag::Node n) {

	if (_meshCache.count(n)) {

		_meshes->add(_crag.id(n), _meshCache[n]);
		return;
	}

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

	_meshCache[n] = mesh;
}

void
MeshViewController::removeMesh(Crag::Node n) {

	_meshes->remove(_crag.id(n));
}
