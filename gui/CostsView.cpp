#include "CostsView.h"
#include <sg_gui/OpenGl.h>

CostsView::CostsView(
		const Crag&  crag,
		const CragVolumes& volumes,
		const Costs& costs,
		std::string name) :
	_crag(crag),
	_volumes(volumes),
	_costs(costs),
	_name(name),
	_showNeighborLinks(false),
	_currentNode(Crag::Invalid) {

	std::vector<double> c;

	for (auto e : _crag.edges())
		c.push_back(_costs.edge[e]);

	if (c.size() > 0) {

		std::nth_element(
				c.begin(),
				c.begin() + c.size()/2,
				c.end());
		double medianCost = *(c.begin() + c.size()/2);

		_costsScale = 1.0/medianCost;

	} else {

		_costsScale = 1.0;
	}
}

void
CostsView::onSignal(sg_gui::Draw& signal) {

	if (!_showNeighborLinks || _currentNode == Crag::Invalid)
		return;

	util::point<float, 3> center = _volumes[_currentNode]->getBoundingBox().center();

	// to see links in 2D, move them in front of the image plane
	float zOffset = -1.5*_volumes[_currentNode]->getResolution().z();

	for (Crag::CragEdge e : _crag.adjEdges(_currentNode)) {

		Crag::CragNode neighbor = _crag.oppositeNode(_currentNode, e);
		float costs = _costs.edge[e];

		util::point<float, 3> neighborCenter = _volumes[neighbor]->getBoundingBox().center();

		glLineWidth(1 + 10*std::abs(costs)*_costsScale);

		float r = (costs < 0 ? 0 : 1);
		float g = 1.0 - r;
		glColor3f(r, g, 0.0);

		glBegin(GL_LINES);
		glVertex3f(center.x(), center.y(), center.z() + zOffset);
		glVertex3f(neighborCenter.x(), neighborCenter.y(), neighborCenter.z() + zOffset);
		glEnd();
	}
}

void
CostsView::onSignal(sg_gui::KeyDown& signal) {

	if (signal.key == sg_gui::keys::N) {

		_showNeighborLinks = !_showNeighborLinks;
		send<sg_gui::ContentChanged>();
	}

	if (signal.key == sg_gui::keys::C) {

		_currentNode = Crag::Invalid;
		send<sg_gui::ContentChanged>();
	}
}

void
CostsView::onSignal(SetCandidate& signal) {

	_currentNode = signal.getCandidate();
	std::cout << _name << " of current candidate: " << _costs.node[signal.getCandidate()] << std::endl;
}

void
CostsView::onSignal(SetEdge& signal) {

	std::cout << _name << " of current edge: " << _costs.edge[signal.getEdge()] << std::endl;
}

