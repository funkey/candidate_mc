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

}

