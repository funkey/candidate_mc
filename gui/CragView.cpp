#include "CragView.h"
#include <util/ProgramOptions.h>

util::ProgramOption optionShowNormals(
		util::_long_name        = "showNormals",
		util::_description_text = "Show the mesh normals.");

util::ProgramOption optionShowOverlayContours(
		util::_long_name        = "showOverlayContours",
		util::_description_text = "If an overlay segmentation was set to show, highlight the contours of regions with the same label.");

CragView::CragView() :
	_normalsView(std::make_shared<NormalsView>()),
	_meshView(std::make_shared<sg_gui::MeshView>()),
	_rawScope(std::make_shared<RawScope>()),
	_labelsScope(std::make_shared<LabelsScope>()),
	_rawView(std::make_shared<sg_gui::VolumeView>()),
	_labelsView(std::make_shared<sg_gui::VolumeView>()),
	_volumeRaysView(std::make_shared<VolumeRaysView>()),
	_alpha(1.0),
	_overlayContourWidth(0) {

	if (optionShowOverlayContours) {

		try {

			_overlayContourWidth = optionShowOverlayContours.as<double>();

		} catch (std::exception& e) {

			// well, that didn't work
			_overlayContourWidth = 1.0;
		}
	}

	_rawScope->add(_rawView);
	_labelsScope->add(_labelsView);

	add(_rawScope);
	add(_labelsScope);
	add(_meshView);

	if (optionShowNormals)
		add(_normalsView);

	add(_volumeRaysView);
}

void
CragView::setVolumeMeshes(std::shared_ptr<sg_gui::Meshes> meshes) {

	_normalsView->setMeshes(meshes);
	_meshView->setMeshes(meshes);
}

void
CragView::setRawVolume(std::shared_ptr<ExplicitVolume<float>> volume) {

	// shift the meshes to be centered on the 2D images
	_meshView->setOffset(util::point<float, 3>(0, 0, -volume->getResolution().z()/2));

	_rawView->setVolume(volume);
}

void
CragView::setLabelVolumes(std::vector<std::shared_ptr<ExplicitVolume<float>>> volumes) {

	_overlays = volumes;
	_currentOverlay = 0;

	if (volumes.size() == 0)
		return;

	_labelsView->setVolume(_overlays[_currentOverlay]);
}

void
CragView::setVolumeRays(std::shared_ptr<VolumeRays> rays) {

	_volumeRaysView->setVolumeRays(rays);
}

void
CragView::onSignal(sg_gui::Draw& signal) {

	if (_overlays.size() == 0 || _overlayContourWidth == 0)
		return;

	Overlay overlay = _overlays[_currentOverlay];

	util::point<float, 3> off = overlay->getOffset();
	util::point<float, 3> res = overlay->getResolution();

	glColor3f(1.0, 0.0, 0.0);
	glLineWidth(_overlayContourWidth);
	glBegin(GL_LINES);
	int z = _labelsView->getCurrentZ();
	for (int x =  0; x < overlay->width() - 1; x++)
	for (int y =  0; y < overlay->height() - 1; y++) {

		float c = (*overlay)(x,   y, z);
		float r = (*overlay)(x+1, y, z);
		float d = (*overlay)(x, y+1, z);

		if (c != r) {

			glVertex3f(off.x() + (x+1)*res.x(), off.y() +     y*res.y(), off.z() + z*res.z());
			glVertex3f(off.x() + (x+1)*res.x(), off.y() + (y+1)*res.y(), off.z() + z*res.z());
		}

		if (c != d) {

			glVertex3f(off.x() +     x*res.x(), off.y() + (y+1)*res.y(), off.z() + z*res.z());
			glVertex3f(off.x() + (x+1)*res.x(), off.y() + (y+1)*res.y(), off.z() + z*res.z());
		}
	}
	glEnd();
}

void
CragView::onSignal(sg_gui::KeyDown& signal) {

	if (signal.key == sg_gui::keys::Tab) {

		_alpha += 0.5;
		if (_alpha > 1.0)
			_alpha = 0;

		if (signal.modifiers & sg_gui::keys::ShiftDown) {

			util::plane<float,3> plane(
					util::point<float,3>(0, 0, _rawView->getVolume()->getOffset().z() + _rawView->getCurrentZ()*_rawView->getVolume()->getResolution().z()),
					util::point<float,3>(0, 0, 1));
			sendInner<sg_gui::SetAlphaPlane>(_alpha, plane, 0.5);

		} else {

			sendInner<sg_gui::ChangeAlpha>(_alpha);
		}
	}

	if (signal.key == sg_gui::keys::L) {

		_rawScope->toggleZBufferWrites();
		_labelsScope->toggleVisibility();
		send<sg_gui::ContentChanged>();
	}

	if (signal.key >= sg_gui::keys::_1 && signal.key <= sg_gui::keys::_0) {

		int index = signal.key - sg_gui::keys::_1;
		if (index < _overlays.size()) {
			_currentOverlay = index;
			_labelsView->setVolume(_overlays[_currentOverlay]);
		}
	}
}
