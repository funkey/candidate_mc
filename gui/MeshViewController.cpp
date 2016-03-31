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

	// n is a leaf node. If we want to show a solution, we have to find the 
	// candidate above (or equal to) n that is part of the solution and show 
	// that.

	if (_solution) {

		LOG_DEBUG(meshviewcontrollerlog) << "asking for solution node" << std::endl;

		while (!_solution->selected(n)) {

			LOG_DEBUG(meshviewcontrollerlog) << "node " << _crag.id(n) << " is not part of solution" << std::endl;

			n = parentOf(n);
			if (n == Crag::Invalid) {

				LOG_DEBUG(meshviewcontrollerlog) << "no more parents" << std::endl;
				return;
			}

			LOG_DEBUG(meshviewcontrollerlog) << "probing parent node " << _crag.id(n) << std::endl;
		}

		LOG_DEBUG(meshviewcontrollerlog) << "node " << _crag.id(n) << " is part of solution" << std::endl;
	}

	if (_meshes->contains(_crag.id(n))) {

		_current = Crag::CragNode();
		removeMesh(n);
		send<sg_gui::SetMeshes>(_meshes);

	} else
		setCurrentMesh(n);

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
MeshViewController::onSignal(sg_gui::KeyDown& signal) {

	if (signal.key == sg_gui::keys::I) {

		LOG_USER(meshviewcontrollerlog) << "enter candidate id: " << std::endl;

		char input[256];
		std::cin.getline(input, 256);

		try {

			unsigned int id = boost::lexical_cast<unsigned int>(input);

			Crag::CragNode n = _crag.nodeFromId(id);
			setCurrentMesh(n);
			_path.clear();

		} catch (std::exception& e) {

			LOG_ERROR(meshviewcontrollerlog) << "invalid input" << std::endl;
			return;
		}
	}

	if (signal.key == sg_gui::keys::C) {

		clearMeshes();
		send<sg_gui::SetMeshes>(_meshes);
	}
}

void
MeshViewController::nextVolume() {

	if (_current == Crag::Invalid)
		return;

	Crag::CragNode parent = parentOf(_current);

	if (parent == Crag::Invalid)
		return;

	_path.push_back(_current);

	replaceCurrentMesh(parent);
}

void
MeshViewController::prevVolume() {

	if (_current == Crag::Invalid)
		return;

	if (_path.size() == 0)
		return;

	Crag::CragNode child = _path.back();
	_path.pop_back();

	replaceCurrentMesh(child);
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
MeshViewController::setCurrentMesh(Crag::CragNode n) {

	LOG_USER(meshviewcontrollerlog)
			<< "showing node with id " << _crag.id(n)
			<< " at " << _volumes[n]->getBoundingBox() << std::endl;

	_current = n;

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

	_neighbors.clear();
	for (Crag::CragEdge e : _crag.adjEdges(_current))
		_neighbors.push_back(e.opposite(_current));
	_currentNeighbor = -1;

	LOG_USER(meshviewcontrollerlog) << "current node has " << _neighbors.size() << " neighbors" << std::endl;

	send<sg_gui::SetMeshes>(_meshes);
	send<SetCandidate>(n);
}

void
MeshViewController::replaceCurrentMesh(Crag::CragNode n) {

	removeMesh(_current);
	setCurrentMesh(n);
}

void
MeshViewController::showNeighbor(Crag::CragNode n) {

	LOG_USER(meshviewcontrollerlog) << "showing neighbor with id " << _crag.id(n) << std::endl;

	addMesh(n);

	send<sg_gui::SetMeshes>(_meshes);
	send<SetCandidate>(n);

	for (Crag::CragEdge e : _crag.adjEdges(_current))
		if (e.opposite(_current) == n) {

			send<SetEdge>(e);
			break;
		}
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
