#include <sg_gui/OpenGl.h>
#include <sg_gui/Colors.h>
#include "EdgeView.h"
#include <util/geometry.hpp>

void
EdgeView::setEdges(std::shared_ptr<Edges> edges) {

	_edges = edges;
	updateRecording();

	send<sg_gui::ContentChanged>();
}

void
EdgeView::setOffset(util::point<float, 3> offset) {

	_offset = offset;
	send<sg_gui::ContentChanged>();
}

void
EdgeView::onSignal(sg_gui::DrawOpaque& /*draw*/) {

	if (_alpha < 1.0)
		return;

	draw();
}

void
EdgeView::onSignal(sg_gui::DrawTranslucent& /*draw*/) {

	if (_alpha == 1.0 || _alpha == 0.0)
		return;

	draw();
}

void
EdgeView::onSignal(sg_gui::QuerySize& signal) {

	if (!_edges)
		return;

	signal.setSize(_edges->getBoundingBox() + _offset);
}

void
EdgeView::onSignal(sg_gui::ChangeAlpha& signal) {

	_alpha = signal.alpha;
	_haveAlphaPlane = false;

	updateRecording();

	send<sg_gui::ContentChanged>();
}

void
EdgeView::onSignal(sg_gui::SetAlphaPlane& signal) {

	_alpha        = signal.alpha;
	_alphaPlane   = signal.plane;
	_alphaFalloff = signal.falloff;
	_haveAlphaPlane = true;

	updateRecording();

	send<sg_gui::ContentChanged>();
}

void
EdgeView::updateRecording() {

	if (!_edges)
		return;


	sg_gui::OpenGl::Guard guard;

	startRecording();

	glPushMatrix();
	glTranslatef(_offset.x(), _offset.y(), _offset.z());

	for (unsigned int id : _edges->getEdgeIds()) {

		// colorize the edge according to its id
		unsigned char cr, cg, cb;
		sg_gui::idToRgb(_edges->getColor(id), cr, cg, cb);
		float r = static_cast<float>(cr)/255.0;
		float g = static_cast<float>(cg)/255.0;
		float b = static_cast<float>(cb)/255.0;

		const Edge& edge = _edges->get(id);

		glBegin(GL_LINES);
		setVertexAlpha(edge.start(), r, g, b);
		glVertex3f(edge.start().x(), edge.start().y(), edge.start().z());
		setVertexAlpha(edge.end(), r, g, b);
		glVertex3f(edge.end().x(), edge.end().y(), edge.end().z());
		glEnd();
	}

	glPopMatrix();

	stopRecording();
}

void
EdgeView::setVertexAlpha(const util::point<float, 3>& p, float r, float g, float b) {

	if (_haveAlphaPlane) {

		double alpha = 1.0 - std::abs(distance(_alphaPlane, p)*_alphaFalloff);
		glColor4f(r, g, b, _alpha*alpha);

	} else {

		glColor4f(r, g, b, _alpha);
	}
}

