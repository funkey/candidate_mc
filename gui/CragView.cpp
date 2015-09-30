#include "CragView.h"
#include <util/ProgramOptions.h>

util::ProgramOption optionShowNormals(
		util::_long_name        = "showNormals",
		util::_description_text = "Show the mesh normals.");

CragView::CragView() :
	_normalsView(std::make_shared<NormalsView>()),
	_meshView(std::make_shared<sg_gui::MeshView>()),
	_rawScope(std::make_shared<RawScope>()),
	_labelsScope(std::make_shared<LabelsScope>()),
	_rawView(std::make_shared<sg_gui::VolumeView>()),
	_labelsView(std::make_shared<sg_gui::VolumeView>()),
	_alpha(1.0) {

	_rawScope->add(_rawView);
	_labelsScope->add(_labelsView);

	add(_rawScope);
	add(_labelsScope);
	add(_meshView);

	if (optionShowNormals)
		add(_normalsView);
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
CragView::setLabelsVolume(std::shared_ptr<ExplicitVolume<float>> volume) {

	_labelsView->setVolume(volume);
}

void
CragView::onSignal(sg_gui::KeyDown& signal) {

	if (signal.key == sg_gui::keys::Tab) {

		_alpha += 0.5;
		if (_alpha > 1.0)
			_alpha = 0;

		sendInner<sg_gui::ChangeAlpha>(_alpha);
	}

	if (signal.key == sg_gui::keys::L) {

		_rawScope->toggleZBufferWrites();
		_labelsScope->toggleVisibility();
		send<sg_gui::ContentChanged>();
	}
}
