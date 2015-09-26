#ifndef CANDIDATE_MC_GUI_SIGNALS_H__
#define CANDIDATE_MC_GUI_SIGNALS_H__

#include <sg_gui/GuiSignals.h>

class SetCandidate : public sg_gui::SetContent {

public:

	typedef SetContent parent_type;

	SetCandidate(Crag::CragNode n) :
			_n(n) {}

	Crag::CragNode getCandidate() const { return _n; }

private:

	Crag::CragNode _n;
};

class SetEdge : public sg_gui::SetContent {

public:

	typedef SetContent parent_type;

	SetEdge(Crag::CragEdge e) :
			_e(e) {}

	Crag::CragEdge getEdge() const { return _e; }

private:

	Crag::CragEdge _e;
};

#endif // CANDIDATE_MC_GUI_SIGNALS_H__

