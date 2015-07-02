#include <boost/lexical_cast.hpp>
#include <util/Logger.h>
#include <util/assert.h>
#include "Hdf5CragStore.h"

logger::LogChannel hdf5storelog("hdf5storelog", "[Hdf5CragStore] ");

void
Hdf5CragStore::saveCrag(const Crag& crag) {

	_hdfFile.root();
	_hdfFile.cd_mk("crag");

	_hdfFile.cd_mk("adjacencies");
	writeGraph(crag.getAdjacencyGraph());

	_hdfFile.cd("/crag");
	_hdfFile.cd_mk("subsets");
	writeDigraph(crag.getSubsetGraph());

	_hdfFile.cd("/crag");
	_hdfFile.cd_mk("volumes");
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
		if (crag.isLeafNode(n))
			writeVolume(crag.getVolume(n), boost::lexical_cast<std::string>(crag.id(n)));
}

void
Hdf5CragStore::retrieveCrag(Crag& crag) {

	_hdfFile.root();
	_hdfFile.cd("crag");

	_hdfFile.cd("adjacencies");
	readGraph(crag.getAdjacencyGraph());

	_hdfFile.cd("/crag");
	_hdfFile.cd("subsets");
	readDigraph(crag.getSubsetGraph());

	_hdfFile.cd("/crag");
	_hdfFile.cd("volumes");

	std::vector<std::string> vols = _hdfFile.ls();

	for (std::string vol : vols) {

		int id = boost::lexical_cast<int>(vol);

		Crag::Node n = crag.nodeFromId(id);
		readVolume(crag.getVolumeMap()[n], vol);

		UTIL_ASSERT(!crag.getBoundingBox(n).isZero());
	}
}

void
Hdf5CragStore::saveNodeFeatures(const Crag& crag, const NodeFeatures& features) {

	_hdfFile.root();
	_hdfFile.cd_mk("crag");
	_hdfFile.cd_mk("features");
	_hdfFile.cd_mk("nodes");

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		const std::vector<double>& f = features[n];

		_hdfFile.write(
				boost::lexical_cast<std::string>(crag.id(n)),
				vigra::ArrayVectorView<double>(f.size(), const_cast<double*>(&f[0])));
	}
}

void
Hdf5CragStore::retrieveNodeFeatures(const Crag& crag, NodeFeatures& features) {

	_hdfFile.root();
	_hdfFile.cd("crag");
	_hdfFile.cd("features");
	_hdfFile.cd("nodes");

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		vigra::ArrayVector<double> f;

		_hdfFile.readAndResize(
				boost::lexical_cast<std::string>(crag.id(n)),
				f);

		features[n].resize(f.size());
		std::copy(f.begin(), f.end(), features[n].begin());
	}
}

void
Hdf5CragStore::saveEdgeFeatures(const Crag& crag, const EdgeFeatures& features) {

	_hdfFile.root();
	_hdfFile.cd_mk("crag");
	_hdfFile.cd_mk("features");
	_hdfFile.cd_mk("edges");

	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e) {

		const std::vector<double>& f = features[e];

		_hdfFile.write(
				boost::lexical_cast<std::string>(crag.id(crag.u(e))) + "-" +
				boost::lexical_cast<std::string>(crag.id(crag.v(e))),
				vigra::ArrayVectorView<double>(f.size(), const_cast<double*>(&f[0])));
	}
}

void
Hdf5CragStore::retrieveEdgeFeatures(const Crag& crag, EdgeFeatures& features) {

	_hdfFile.root();
	_hdfFile.cd("crag");
	_hdfFile.cd("features");
	_hdfFile.cd("edges");

	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e) {

		vigra::ArrayVector<double> f;

		_hdfFile.readAndResize(
				boost::lexical_cast<std::string>(crag.id(crag.u(e))) + "-" +
				boost::lexical_cast<std::string>(crag.id(crag.v(e))),
				f);

		features[e].resize(f.size());
		std::copy(f.begin(), f.end(), features[e].begin());
	}
}

void
Hdf5CragStore::saveNodeFeaturesMinMax(
		const std::vector<double>& min,
		const std::vector<double>& max) {

	_hdfFile.root();
	_hdfFile.cd_mk("crag");
	_hdfFile.cd_mk("features");

	_hdfFile.write(
			"node_features_min",
			vigra::ArrayVectorView<double>(min.size(), const_cast<double*>(&min[0])));
	_hdfFile.write(
			"node_features_max",
			vigra::ArrayVectorView<double>(max.size(), const_cast<double*>(&max[0])));
}

void
Hdf5CragStore::retrieveNodeFeaturesMinMax(
			std::vector<double>& min,
			std::vector<double>& max) {

	_hdfFile.root();
	_hdfFile.cd("crag");
	_hdfFile.cd("features");

	vigra::ArrayVector<double> f;
	_hdfFile.readAndResize(
			"node_features_min",
			f);
	min.resize(f.size());
	std::copy(f.begin(), f.end(), min.begin());
	_hdfFile.write(
			"node_features_max",
			f);
	max.resize(f.size());
	std::copy(f.begin(), f.end(), max.begin());
}

void
Hdf5CragStore::saveEdgeFeaturesMinMax(
		const std::vector<double>& min,
		const std::vector<double>& max) {

	_hdfFile.root();
	_hdfFile.cd_mk("crag");
	_hdfFile.cd_mk("features");

	_hdfFile.write(
			"edge_features_min",
			vigra::ArrayVectorView<double>(min.size(), const_cast<double*>(&min[0])));
	_hdfFile.write(
			"edge_features_max",
			vigra::ArrayVectorView<double>(max.size(), const_cast<double*>(&max[0])));
}

void
Hdf5CragStore::retrieveEdgeFeaturesMinMax(
			std::vector<double>& min,
			std::vector<double>& max) {

	_hdfFile.root();
	_hdfFile.cd("crag");
	_hdfFile.cd("features");

	vigra::ArrayVector<double> f;
	_hdfFile.readAndResize(
			"edge_features_min",
			f);
	min.resize(f.size());
	std::copy(f.begin(), f.end(), min.begin());
	_hdfFile.write(
			"edge_features_max",
			f);
	max.resize(f.size());
	std::copy(f.begin(), f.end(), max.begin());
}

void
Hdf5CragStore::saveSegmentation(
		const Crag&                              crag,
		const std::vector<std::set<Crag::Node>>& segmentation,
		std::string                              name) {

	_hdfFile.root();
	_hdfFile.cd_mk("segmentations");
	_hdfFile.cd_mk(name);

	for (unsigned int i = 0; i < segmentation.size(); i++) {

		std::string segmentName = "segment_";
		segmentName += boost::lexical_cast<std::string>(i);

		vigra::ArrayVector<int> nodes;
		for (Crag::Node node : segmentation[i])
			nodes.push_back(crag.id(node));
		_hdfFile.write(
				segmentName,
				nodes);
	}
}

void
Hdf5CragStore::retrieveSegmentation(
		const Crag&                        crag,
		std::vector<std::set<Crag::Node>>& segmentation,
		std::string                        name) {

	_hdfFile.root();
	_hdfFile.cd("segmentations");
	_hdfFile.cd(name);

	std::vector<std::string> segmentNames = _hdfFile.ls();

	for (std::string segmentName : segmentNames) {

		vigra::ArrayVector<int> ids;
		_hdfFile.readAndResize(
				segmentName,
				ids);

		std::set<Crag::Node> nodes;
		for (int id : ids)
			nodes.insert(crag.nodeFromId(id));

		segmentation.push_back(nodes);
	}
}
