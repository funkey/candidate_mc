#ifndef CANDIDATE_MC_GUI_SIGNALS_H__
#define CANDIDATE_MC_GUI_SIGNALS_H__

#include <sg_gui/GuiSignals.h>

class SetCandidate : public sg_gui::SetContent {

public:

	typedef SetContent parent_type;

	SetCandidate(Crag::Node n) :
			_n(n) {}

	Crag::Node getCandidate() const { return _n; }

private:

	Crag::Node _n;
};

class SetEdge : public sg_gui::SetContent {

public:

	typedef SetContent parent_type;

	SetEdge(Crag::Edge e) :
			_e(e) {}

	Crag::Edge getEdge() const { return _e; }

private:

	Crag::Edge _e;
};

#endif // CANDIDATE_MC_GUI_SIGNALS_H__

