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
	_currentNeighbor(-1) {}

void
MeshViewController::loadMeshes(const std::vector<Crag::CragNode>& nodes) {

	for (Crag::CragNode node : nodes)
		addMesh(node);

	send<sg_gui::SetMeshes>(_meshes);
}

void
MeshViewController::setSolution(std::shared_ptr<CragSolution> solution) {

	_solution = solution;
}

void
MeshViewController::onSignal(sg_gui::VolumePointSelected& signal) {

	unsigned int x, y, z;
	_labels->getDiscreteCoordinates(
			signal.position().x(),
			signal.position().y(),
			signal.position().z(),
			x, y, z);

	Crag::CragNode n = _crag.nodeFromId((*_labels)(x, y, z));

	if (n == Crag::Invalid)
		return;

	LOG_DEBUG(meshviewcontrollerlog) << "selected node " << _crag.id(n) << std::endl;

	clearPath();
	setCurrentCandidate(n);

	// n is a leaf node. If we want to show a solution, we have to find the 
	// candidate above (or equal to) n that is part of the solution and show 
	// that.

	if (_solution) {

		LOG_DEBUG(meshviewcontrollerlog) << "going to next solution node" << std::endl;

		while (!_solution->selected(_currentCandidate)) {

			LOG_DEBUG(meshviewcontrollerlog) << "node " << _crag.id(_currentCandidate) << " is not part of solution" << std::endl;

			if (!higherCandidate()) {

				LOG_DEBUG(meshviewcontrollerlog) << "no more parents" << std::endl;
				setCurrentCandidate(n);
				return;
			}
		}

		LOG_DEBUG(meshviewcontrollerlog) << "node " << _crag.id(_currentCandidate) << " is part of solution" << std::endl;
	}
}

void
MeshViewController::onSignal(sg_gui::MouseDown& signal) {

	if (signal.processed)
		return;

	if (signal.button == sg_gui::buttons::WheelUp && signal.modifiers & sg_gui::keys::ShiftDown)
		higherCandidate();
	else if (signal.button == sg_gui::buttons::WheelDown && signal.modifiers & sg_gui::keys::ShiftDown)
		lowerCandidate();
	else if (signal.button == sg_gui::buttons::WheelUp && signal.modifiers & sg_gui::keys::AltDown)
		nextNeighbor();
	else if (signal.button == sg_gui::buttons::WheelDown && signal.modifiers & sg_gui::keys::AltDown)
		prevNeighbor();
	else
		return;

	signal.processed = true;
}

void
MeshViewController::onSignal(sg_gui::KeyDown& signal) {

	if (signal.key == sg_gui::keys::I) {

		LOG_USER(meshviewcontrollerlog) << "enter candidate id: " << std::endl;

		char input[256];
		std::cin.getline(input, 256);

		try {

			unsigned int id = boost::lexical_cast<unsigned int>(input);

			Crag::CragNode n = _crag.nodeFromId(id);
			clearPath();
			setCurrentCandidate(n);

		} catch (std::exception& e) {

			LOG_ERROR(meshviewcontrollerlog) << "invalid input" << std::endl;
			return;
		}
	}

	if (signal.key == sg_gui::keys::C) {

		clearCandidates();
		send<sg_gui::SetMeshes>(_meshes);
	}
}

bool
MeshViewController::higherCandidate() {

	if (_currentCandidate == Crag::Invalid)
		return false;

	Crag::CragNode parent = parentOf(_currentCandidate);

	if (parent == Crag::Invalid)
		return false;

	_path.push_back(_currentCandidate);

	replaceCurrentCandidate(parent);

	return true;
}

bool
MeshViewController::lowerCandidate() {

	if (_currentCandidate == Crag::Invalid)
		return false;

	if (_path.size() == 0)
		return false;

	Crag::CragNode child = _path.back();
	_path.pop_back();

	replaceCurrentCandidate(child);

	return true;
}

void
MeshViewController::replaceCurrentCandidate(Crag::CragNode n) {

	if (_currentCandidate != Crag::Invalid)
		removeMesh(_currentCandidate);
	setCurrentCandidate(n);
}

void
MeshViewController::setCurrentCandidate(Crag::CragNode n) {

	LOG_USER(meshviewcontrollerlog)
			<< "showing node with id " << _crag.id(n)
			<< " at " << _volumes[n]->getBoundingBox() << std::endl;

	_currentCandidate = n;

	if (_solution && _solution->label(n) != 0) {

		// find all other nodes that are in a connected component with n and
		// show all of them
		int label = _solution->label(n);

		LOG_DEBUG(meshviewcontrollerlog) << "label of selected node is " << label << std::endl;

		for (Crag::CragNode m : _crag.nodes())
			if (_solution->label(m) == label) {

				LOG_DEBUG(meshviewcontrollerlog) << "adding node " << _crag.id(m) << " as well" << std::endl;
				addMesh(m);
			}

	} else {

		addMesh(n);
	}

	replaceCurrentNeighbor(-1);
	_neighbors.clear();
	for (Crag::CragEdge e : _crag.adjEdges(_currentCandidate))
		_neighbors.push_back(e.opposite(_currentCandidate));

	send<sg_gui::SetMeshes>(_meshes);
	send<SetCandidate>(n);

	LOG_USER(meshviewcontrollerlog) << "current node has " << _neighbors.size() << " neighbors" << std::endl;
}

void
MeshViewController::nextNeighbor() {

	if (_neighbors.size() == 0)
		return;

	int n = std::min((int)(_neighbors.size() - 1), _currentNeighbor + 1);
	replaceCurrentNeighbor(n);
}

void
MeshViewController::prevNeighbor() {

	if (_neighbors.size() == 0)
		return;

	int n = std::max(0, _currentNeighbor - 1);
	replaceCurrentNeighbor(n);
}

void
MeshViewController::replaceCurrentNeighbor(int index) {

	if (_currentCandidate == Crag::Invalid)
		return;

	if (_currentNeighbor >= 0)
		removeMesh(_neighbors[_currentNeighbor]);

	_currentNeighbor = index;

	if (_currentNeighbor == -1)
		return;

	addMesh(_neighbors[_currentNeighbor]);
	send<sg_gui::SetMeshes>(_meshes);

	send<SetCandidate>(_neighbors[_currentNeighbor]);

	for (Crag::CragEdge e : _crag.adjEdges(_currentCandidate))
		if (e.opposite(_currentCandidate) == _neighbors[_currentNeighbor]) {

			send<SetEdge>(e);
			break;
		}

	LOG_DEBUG(meshviewcontrollerlog) << "new current neighbor is " << index << std::endl;
}

void
MeshViewController::addMesh(Crag::CragNode n) {

	int id = _crag.id(n);
	int color;

	if (_solution)
		color = _solution->label(n) + 1;
	else
		color = _crag.id(n);

	if (_meshCache.count(n)) {

		_meshes->add(id, _meshCache[n], color);
		return;
	}

	const CragVolume& volume = *_volumes[n];

	typedef ExplicitVolumeAdaptor<CragVolume> Adaptor;
	Adaptor adaptor(volume);

	sg_gui::MarchingCubes<Adaptor> marchingCubes;
	std::shared_ptr<sg_gui::Mesh> mesh = marchingCubes.generateSurface(
			adaptor,
			sg_gui::MarchingCubes<Adaptor>::AcceptAbove(0),
			optionCubeSize,
			optionCubeSize,
			optionCubeSize);
	_meshes->add(id, mesh, color);

	_meshCache[n] = mesh;

	LOG_DEBUG(meshviewcontrollerlog) << "mesh for node " << _crag.id(n) << " added" << std::endl;
}

void
MeshViewController::removeMesh(Crag::CragNode n) {

	if (_solution) {

		// find all other nodes that are in a connected component with n and 
		// remove all of them
		int label = _solution->label(n);

		LOG_DEBUG(meshviewcontrollerlog) << "label of selected node is " << label << std::endl;

		for (Crag::CragNode m : _crag.nodes())
			if (_solution->label(m) == label) {

				LOG_DEBUG(meshviewcontrollerlog) << "removing node " << _crag.id(m) << " as well" << std::endl;
				_meshes->remove(_crag.id(m));
			}

	} else {

		_meshes->remove(_crag.id(n));
	}

	LOG_DEBUG(meshviewcontrollerlog) << "mesh for node " << _crag.id(n) << " removed" << std::endl;
}

void
MeshViewController::clearPath() {

	_path.clear();
}

void
MeshViewController::clearCandidates() {

	clearPath();
	_currentCandidate = Crag::Invalid;
	_currentNeighbor = -1;
	clearMeshes();
}

void
MeshViewController::clearMeshes() {

	_meshes->clear();
}

Crag::CragNode
MeshViewController::parentOf(Crag::CragNode n) {

	// Find the parent of node n, but don't accept assignment nodes (they are 
	// not really parents).

	if (_crag.outArcs(n).size() == 0)
		return Crag::Invalid;

	for (Crag::CragArc a : _crag.outArcs(n))
		if (_crag.type(a.target()) != Crag::AssignmentNode)
			return a.target();

	return Crag::Invalid;
}
