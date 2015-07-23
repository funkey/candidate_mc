#include "MeshViewController.h"
#include <sg_gui/MarchingCubes.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>

logger::LogChannel meshviewcontrollerlog("meshviewcontrollerlog", "[MeshViewController] ");

util::ProgramOption optionCubeSize(
		util::_long_name        = "cubeSize",
		util::_description_text = "The size of a cube for the marching cubes visualization.",
		util::_default_value    = 10);

MeshViewController::MeshViewController(
		const Crag&                            crag,
		const CragVolumes&                     volumes,
		std::shared_ptr<ExplicitVolume<float>> labels) :
	_crag(crag),
	_volumes(volumes),
	_labels(labels),
	_meshes(std::make_shared<sg_gui::Meshes>()),
	_current(lemon::INVALID),
	_currentNeighbor(-1) {}

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

	if (_meshes->contains(_crag.id(n))) {

		_current = lemon::INVALID;
		removeMesh(n);

	} else
		showSingleMesh(n);

	_path.clear();
}

void
MeshViewController::onSignal(sg_gui::MouseDown& signal) {

	if (signal.processed)
		return;

	if (signal.button == sg_gui::buttons::WheelUp && signal.modifiers & sg_gui::keys::ShiftDown)
		nextVolume();
	else if (signal.button == sg_gui::buttons::WheelDown && signal.modifiers & sg_gui::keys::ShiftDown)
		prevVolume();
	else if (signal.button == sg_gui::buttons::WheelUp && signal.modifiers & sg_gui::keys::AltDown)
		nextNeighbor();
	else if (signal.button == sg_gui::buttons::WheelDown && signal.modifiers & sg_gui::keys::AltDown)
		prevNeighbor();
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

	showSingleMesh(parent);
}

void
MeshViewController::prevVolume() {

	if (_current == lemon::INVALID)
		return;

	if (_path.size() == 0)
		return;

	Crag::Node child = _path.back();
	_path.pop_back();

	showSingleMesh(child);
}

void
MeshViewController::nextNeighbor() {

	if (_neighbors.size() == 0)
		return;

	if (_currentNeighbor >= 0)
		removeMesh(_neighbors[_currentNeighbor]);

	_currentNeighbor = std::min((int)(_neighbors.size() - 1), _currentNeighbor + 1);

	showNeighbor(_neighbors[_currentNeighbor]);
}

void
MeshViewController::prevNeighbor() {

	if (_neighbors.size() == 0)
		return;

	if (_currentNeighbor >= 0)
		removeMesh(_neighbors[_currentNeighbor]);

	_currentNeighbor = std::max(0, _currentNeighbor - 1);

	showNeighbor(_neighbors[_currentNeighbor]);
}

void
MeshViewController::showSingleMesh(Crag::Node n) {

	LOG_USER(meshviewcontrollerlog) << "showing node with id " << _crag.id(n) << std::endl;

	_current = n;
	_meshes->clear();
	addMesh(n);

	_neighbors.clear();
	for (Crag::IncEdgeIt e(_crag, _current); e != lemon::INVALID; ++e)
		_neighbors.push_back(_crag.getAdjacencyGraph().oppositeNode(_current, e));
	_currentNeighbor = -1;

	LOG_USER(meshviewcontrollerlog) << "current node has " << _neighbors.size() << " neighbors" << std::endl;

	send<sg_gui::SetMeshes>(_meshes);
	send<SetCandidate>(n);
}

void
MeshViewController::showNeighbor(Crag::Node n) {

	LOG_USER(meshviewcontrollerlog) << "showing neighbor with id " << _crag.id(n) << std::endl;

	addMesh(n);

	send<sg_gui::SetMeshes>(_meshes);

	for (Crag::IncEdgeIt e(_crag, _current); e != lemon::INVALID; ++e)
		if (_crag.getAdjacencyGraph().oppositeNode(_current, e) == _neighbors[_currentNeighbor]) {

			send<SetEdge>(e);
			break;
		}
}

void
MeshViewController::addMesh(Crag::Node n) {

	if (_meshCache.count(n)) {

		_meshes->add(_crag.id(n), _meshCache[n]);
		return;
	}

	const CragVolume& volume = *_volumes[n];

	typedef ExplicitVolumeAdaptor<CragVolume> Adaptor;
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
