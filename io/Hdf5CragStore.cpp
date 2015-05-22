#include <boost/lexical_cast.hpp>
#include <util/Logger.h>
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
		if (!crag.getVolumes()[n].getBoundingBox().isZero())
			writeVolume(crag.getVolumes()[n], boost::lexical_cast<std::string>(crag.id(n)));
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
		readVolume(crag.getVolumes()[n], vol);
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

}

void
Hdf5CragStore::retrieveEdgeFeatures(const Crag& crag, EdgeFeatures& features) {

}
