#ifndef CANDIDATE_MC_GUI_EDGES_H__
#define CANDIDATE_MC_GUI_EDGES_H__

#include <imageprocessing/Volume.h>
#include <Edge.h>

class Edges : public Volume {

public:

	Edges() {}

	void add(unsigned int id, Edge edge, int color = -1) {

		_edges[id] = edge;
		_colors[id] = (color < 0 ? id : color);

		setBoundingBoxDirty();
	}

	void remove(unsigned int id) {

		_edges.erase(id);
		_colors.erase(id);

		setBoundingBoxDirty();
	}

	const Edge& get(unsigned int id) const {

		return _edges.at(id);
	}

	int getColor(unsigned int id) {

		return _colors[id];
	}

	const std::vector<unsigned int> getEdgeIds() const {

		std::vector<unsigned int> ids;
		for (auto& p : _edges)
			ids.push_back(p.first);
		return ids;
	}

	void clear() { _edges.clear(); _colors.clear(); setBoundingBoxDirty(); }

	bool contains(unsigned int id) const { return _edges.count(id); }

private:

	util::box<float,3> computeBoundingBox() const {

		util::box<float,3> boundingBox;

		for (auto& p : _edges) {

			const Edge& edge = p.second;

			boundingBox.fit(edge.start());
			boundingBox.fit(edge.end());
		}

		return boundingBox;
	}

	std::map<unsigned int, Edge> _edges;
	std::map<unsigned int, int>  _colors;
};


#endif // CANDIDATE_MC_GUI_EDGES_H__

