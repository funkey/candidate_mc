#ifndef CANDIDATE_MC_GUI_EDGE_H__
#define CANDIDATE_MC_GUI_EDGE_H__

class Edge {

public:

	Edge() {}

	Edge(
			util::point<float, 3> start,
			util::point<float, 3> end) :
		_start(start),
		_end(end) {}

	const util::point<float, 3>& start() const { return _start; }
	const util::point<float, 3>& end()   const { return _end; }

private:

	util::point<float, 3> _start;
	util::point<float, 3> _end;
};

#endif // CANDIDATE_MC_GUI_EDGE_H__

