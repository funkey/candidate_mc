#ifndef CANDIDATE_MC_GUI_MESH_VIEW_H__
#define CANDIDATE_MC_GUI_MESH_VIEW_H__

#include <scopegraph/Agent.h>
#include <sg_gui/GuiSignals.h>
#include <sg_gui/ViewSignals.h>
#include <sg_gui/RecordableView.h>
#include "Edges.h"

class SetEdges : public sg_gui::SetContent {

public:

	typedef sg_gui::SetContent parent_type;

	SetEdges(std::shared_ptr<Edges> edges) :
			_edges(edges) {}

	std::shared_ptr<Edges> getEdges() { return _edges; }

private:

	std::shared_ptr<Edges> _edges;
};

class EdgeView :
		public sg::Agent<
			EdgeView,
			sg::Accepts<
					SetEdges,
					sg_gui::DrawOpaque,
					sg_gui::DrawTranslucent,
					sg_gui::QuerySize,
					sg_gui::ChangeAlpha,
					sg_gui::SetAlphaPlane
			>,
			sg::Provides<
					sg_gui::ContentChanged
			>
		>,
		public sg_gui::RecordableView {

public:

	EdgeView() : _alpha(1.0), _haveAlphaPlane(false) {}

	void setEdges(std::shared_ptr<Edges> edges);

	void setOffset(util::point<float, 3> offset);

	void onSignal(SetEdges& signal) { setEdges(signal.getEdges()); }

	void onSignal(sg_gui::DrawOpaque& signal);

	void onSignal(sg_gui::DrawTranslucent& signal);

	void onSignal(sg_gui::QuerySize& signal);

	void onSignal(sg_gui::ChangeAlpha& signal);

	void onSignal(sg_gui::SetAlphaPlane& signal);

private:

	void updateRecording();

	void setVertexAlpha(const util::point<float, 3>& p, float r, float g, float b);

	std::shared_ptr<Edges> _edges;

	double _alpha;
	util::plane<float, 3> _alphaPlane;
	double _alphaFalloff;
	bool _haveAlphaPlane;

	util::point<float, 3> _offset;
};

#endif // CANDIDATE_MC_GUI_MESH_VIEW_H__

