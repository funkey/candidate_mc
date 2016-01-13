#ifndef CANDIDATE_MC_GUI_VOLUME_RAYS_VIEW_H__
#define CANDIDATE_MC_GUI_VOLUME_RAYS_VIEW_H__

#include <features/VolumeRays.h>
#include <scopegraph/Agent.h>
#include <sg_gui/RecordableView.h>
#include <sg_gui/GuiSignals.h>
#include <sg_gui/KeySignals.h>
#include "Signals.h"

class VolumeRaysView :
		public sg::Agent<
				VolumeRaysView,
				sg::Accepts<
						SetCandidate,
						sg_gui::DrawTranslucent,
						sg_gui::QuerySize,
						sg_gui::KeyDown
				>,
				sg::Provides<
						sg_gui::ContentChanged
				>
		>,
		public sg_gui::RecordableView {

public:

	VolumeRaysView() : _visible(false) {}

	void setVolumeRays(std::shared_ptr<VolumeRays> rays) { _rays = rays; updateRecording(); }

	void onSignal(SetCandidate& draw);

	void onSignal(sg_gui::DrawTranslucent& draw);

	void onSignal(sg_gui::QuerySize& signal);

	void onSignal(sg_gui::KeyDown& signal);

private:

	void updateRecording();

	std::shared_ptr<VolumeRays> _rays;

	std::set<Crag::CragNode> _visibleCandidates;

	bool _visible;
};

#endif // CANDIDATE_MC_GUI_VOLUME_RAYS_VIEW_H__

