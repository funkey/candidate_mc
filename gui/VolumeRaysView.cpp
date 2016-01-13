#include "VolumeRaysView.h"
#include <sg_gui/OpenGl.h>

void
VolumeRaysView::onSignal(SetCandidate& signal) {

	// just show the rays of the most recently selected candidate
	_visibleCandidates.clear();
	_visibleCandidates.insert(signal.getCandidate());
	updateRecording();
}

void
VolumeRaysView::onSignal(sg_gui::DrawTranslucent& /*draw*/) {

	draw();
}

void
VolumeRaysView::onSignal(sg_gui::QuerySize& signal) {

	if (!_rays)
		return;

	signal.setSize(_rays->getBoundingBox());
}

void
VolumeRaysView::onSignal(sg_gui::KeyDown& signal) {

	if (signal.key == sg_gui::keys::V)
		_visible = !_visible;

	updateRecording();
}

void
VolumeRaysView::updateRecording() {

	sg_gui::OpenGl::Guard guard;

	startRecording();

	if (_visible) {

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);
		glLineWidth(1.0);
		glColor4f(0, 0, 0, 0.25);

		for (Crag::CragNode n : _visibleCandidates) {

			for (const util::ray<float, 3>& ray : (*_rays)[n]) {

				glBegin(GL_LINES);
				glVertex3f(
						ray.position().x(),
						ray.position().y(),
						ray.position().z());
				glVertex3f(
						ray.position().x() + ray.direction().x(),
						ray.position().y() + ray.direction().y(),
						ray.position().z() + ray.direction().z()); 
				glEnd();
			}
		}
	}

	stopRecording();

	send<sg_gui::ContentChanged>();
}

