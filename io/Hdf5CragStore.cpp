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
	Hdf5GraphWriter::writeNodeMap(crag, crag.nodeTypes(), "node_types", Hdf5GraphWriter::DefaultConverter<Crag::NodeType, int>());

	std::vector<int> edgeTypes;
	for (Crag::CragEdge e : crag.edges()) {

		edgeTypes.push_back(crag.id(e.u()));
		edgeTypes.push_back(crag.id(e.v()));
		edgeTypes.push_back(crag.type(e));
	}
	if (edgeTypes.size() > 0)
		_hdfFile.write(
				"edge_types",
				vigra::ArrayVectorView<int>(edgeTypes.size(), const_cast<int*>(&edgeTypes[0])));

	_hdfFile.cd("/crag");
	_hdfFile.cd_mk("grid_graph");
	vigra::ArrayVector<int> shape(3);
	shape[0] = crag.getGridGraph().shape()[0];
	shape[1] = crag.getGridGraph().shape()[1];
	shape[2] = crag.getGridGraph().shape()[2];
	_hdfFile.write("shape", shape);

	_hdfFile.cd("/crag");
	_hdfFile.cd_mk("affiliated_edges");
	int numEdges = 0;

	// list of affilitated edges:
	//
	// u v n id_1 ... id_n
	//
	// (u, v) ajacency edge
	// n      number of affiliated edges
	// id_i   id of ith affiliated edge
	std::vector<int> aeIds;
	for (Crag::CragEdge e : crag.edges()) {

		if (!crag.isLeafEdge(e))
			continue;

		if (numEdges%100 == 0)
			LOG_USER(hdf5storelog) << logger::delline << numEdges << " affiliated egde lists prepared" << std::flush;

		aeIds.push_back(crag.id(crag.u(e)));
		aeIds.push_back(crag.id(crag.v(e)));
		aeIds.push_back(crag.getAffiliatedEdges(e).size());
		for (vigra::GridGraph<3>::Edge ae : crag.getAffiliatedEdges(e))
			aeIds.push_back(crag.getGridGraph().id(ae));

		numEdges++;
	}
	LOG_USER(hdf5storelog) << logger::delline << numEdges << " affiliated egde lists prepared" << std::endl;

	if (aeIds.size() == 0)
		return;

	LOG_USER(hdf5storelog) << "writing affiliated edge lists..." << std::flush;
	_hdfFile.write(
			"list",
			vigra::ArrayVectorView<int>(aeIds.size(), const_cast<int*>(&aeIds[0])));
	LOG_USER(hdf5storelog) << " done." << std::endl;
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
	Hdf5GraphReader::readNodeMap(crag, crag.nodeTypes(), "node_types", Hdf5GraphReader::DefaultConverter<int, Crag::NodeType>());

	if (_hdfFile.existsDataset("edge_types")) {

		vigra::ArrayVector<int> edgeTypes;
		_hdfFile.readAndResize(
				"edge_types",
				edgeTypes);

		for (unsigned int i = 0; i < edgeTypes.size();) {

			Crag::Node u = crag.nodeFromId(edgeTypes[i]);
			Crag::Node v = crag.nodeFromId(edgeTypes[i+1]);
			Crag::EdgeType type = static_cast<Crag::EdgeType>(edgeTypes[i+2]);
			i += 3;

			// find edge in CRAG and set type
			for (Crag::IncEdgeIt e(crag, u); e != lemon::INVALID; ++e)
				if (crag.getAdjacencyGraph().oppositeNode(u, e) == v) {

					crag.edgeTypes()[e] = type;
					break;
				}
		}
	}

	try {

		_hdfFile.cd("/crag");
		_hdfFile.cd("grid_graph");
		vigra::ArrayVector<int> s;
		_hdfFile.readAndResize("shape", s);
		vigra::Shape3 shape;
		shape[0] = s[0];
		shape[1] = s[1];
		shape[2] = s[2];
		vigra::GridGraph<3> gridGraph(shape, vigra::DirectNeighborhood);
		crag.setGridGraph(gridGraph);

		_hdfFile.cd("/crag");
		_hdfFile.cd("affiliated_edges");

		if (!_hdfFile.existsDataset("list"))
			return;

		vigra::ArrayVector<int> aeIds;
		_hdfFile.readAndResize(
				"list",
				aeIds);

		for (unsigned int i = 0; i < aeIds.size();) {

			Crag::CragNode u = crag.nodeFromId(aeIds[i]);
			Crag::CragNode v = crag.nodeFromId(aeIds[i+1]);
			int n = aeIds[i+2];
			i += 3;

			std::vector<vigra::GridGraph<3>::Edge> affiliatedEdges;
			for (int j = 0; j < n; j++)
				affiliatedEdges.push_back(crag.getGridGraph().edgeFromId(aeIds[i+j]));
			i += n;

			// find edge in CRAG and set affiliated edge list
			if (affiliatedEdges.size()  > 0)
				for (Crag::CragEdge e : crag.adjEdges(u))
					if (crag.getAdjacencyGraph().oppositeNode(u, e) == v) {

						crag.setAffiliatedEdges(e, affiliatedEdges);
						break;
					}
		}

	} catch (std::exception& e) {

		LOG_USER(hdf5storelog) << "no grid-graph description found" << std::endl;
	}
}

void
Hdf5CragStore::saveVolumes(const CragVolumes& volumes) {

	_hdfFile.cd_mk("/crag");
	_hdfFile.cd_mk("volumes");

	std::vector<unsigned char> serialized;
	std::vector<int> meta;
	std::vector<float> offsets;
	std::vector<float> resolutions;

	int numNodes = 0;
	for (Crag::NodeIt n(volumes.getCrag()); n != lemon::INVALID; ++n) {

		// only store leaf node volumes
		if (!volumes.getCrag().isLeafNode(n))
			continue;

		if (numNodes%100 == 0)
			LOG_USER(hdf5storelog) << logger::delline << numNodes << " node volumes prepared for writing" << std::flush;

		const CragVolume& volume = *volumes[n];
		meta.push_back(volumes.getCrag().id(n));
		meta.push_back(volume.width());
		meta.push_back(volume.height());
		meta.push_back(volume.depth());
		offsets.push_back(volume.getOffset().x());
		offsets.push_back(volume.getOffset().y());
		offsets.push_back(volume.getOffset().z());
		resolutions.push_back(volume.getResolution().x());
		resolutions.push_back(volume.getResolution().y());
		resolutions.push_back(volume.getResolution().z());
		std::copy(volume.data().begin(), volume.data().end(), std::back_inserter(serialized));

		numNodes++;
	}

	LOG_USER(hdf5storelog) << logger::delline << numNodes << " node volumes prepared for writing" << std::endl;

	LOG_USER(hdf5storelog) << "writing node volumes... " << std::flush;

	_hdfFile.write(
			"serialized",
			vigra::ArrayVectorView<unsigned char>(serialized.size(), const_cast<unsigned char*>(&serialized[0])));
	_hdfFile.write(
			"meta",
			vigra::ArrayVectorView<int>(meta.size(), const_cast<int*>(&meta[0])));
	_hdfFile.write(
			"offsets",
			vigra::ArrayVectorView<float>(offsets.size(), const_cast<float*>(&offsets[0])));
	_hdfFile.write(
			"resolutions",
			vigra::ArrayVectorView<float>(resolutions.size(), const_cast<float*>(&resolutions[0])));

	LOG_USER(hdf5storelog) << "done." << std::endl;
}

void
Hdf5CragStore::retrieveVolumes(CragVolumes& volumes) {

	_hdfFile.root();
	_hdfFile.cd("/crag");
	_hdfFile.cd("volumes");

	vigra::MultiArray<1, unsigned char> serialized;
	vigra::MultiArray<1, int> meta;
	vigra::MultiArray<1, float> offsets;
	vigra::MultiArray<1, float> resolutions;

	_hdfFile.readAndResize("serialized", serialized);
	_hdfFile.readAndResize("meta", meta);
	_hdfFile.readAndResize("offsets", offsets);
	_hdfFile.readAndResize("resolutions", resolutions);

	UTIL_ASSERT_REL(meta.size() % 4, ==, 0);
	UTIL_ASSERT_REL(offsets.size() % 3, ==, 0);
	UTIL_ASSERT_REL(resolutions.size() % 3, ==, 0);
	UTIL_ASSERT_REL(meta.size()/4, ==, offsets.size()/3);
	UTIL_ASSERT_REL(meta.size()/4, ==, resolutions.size()/3);

	auto si = serialized.begin();
	std::size_t oi = 0;
	std::size_t ri = 0;
	for (int i = 0; i < meta.size();) {

		int id = meta[i++];
		int width = meta[i++];
		int height = meta[i++];
		int depth = meta[i++];

		std::shared_ptr<CragVolume> volume = std::make_shared<CragVolume>(width, height, depth);
		std::copy(si, si + width*height*depth, volume->data().begin());
		si += width*height*depth;

		float x = resolutions[ri++];
		float y = resolutions[ri++];
		float z = resolutions[ri++];
		volume->setResolution(x, y, z);
		x = offsets[oi++];
		y = offsets[oi++];
		z = offsets[oi++];
		volume->setOffset(x, y, z);

		Crag::Node n = volumes.getCrag().nodeFromId(id);
		volumes.setVolume(n, volume);

		UTIL_ASSERT(!volume->getBoundingBox().isZero());
	}
}

void
Hdf5CragStore::saveNodeFeatures(const Crag& crag, const NodeFeatures& features) {

	LOG_USER(hdf5storelog) << "saving node features... " << std::flush;

	_hdfFile.root();
	_hdfFile.cd_mk("crag");
	_hdfFile.cd_mk("features");

	for (Crag::NodeType type : Crag::NodeTypes) {

		int numNodes = 0;
		for (Crag::CragNode n : crag.nodes())
			if (crag.type(n) == type)
				numNodes++;

		if (numNodes == 0)
			continue;

		vigra::MultiArray<2, double> allFeatures(vigra::Shape2(features.dims(type) + 1, numNodes));

		int nodeNum = 0;
		for (Crag::CragNode n : crag.nodes()) {

			if (crag.type(n) != type)
				continue;

			allFeatures(0, nodeNum) = crag.id(n);
			std::copy(
					features[n].begin(),
					features[n].end(),
					allFeatures.bind<1>(nodeNum).begin() + 1);
			nodeNum++;
		}

		_hdfFile.write(std::string("nodes_") + boost::lexical_cast<std::string>(type), allFeatures);
	}

	LOG_USER(hdf5storelog) << "done." << std::endl;
}

void
Hdf5CragStore::retrieveNodeFeatures(const Crag& crag, NodeFeatures& features) {

	_hdfFile.root();
	_hdfFile.cd("crag");
	_hdfFile.cd("features");

	vigra::MultiArray<2, double> allFeatures;

	for (Crag::NodeType type : Crag::NodeTypes) {

		if (!_hdfFile.existsDataset(std::string("nodes_") + boost::lexical_cast<std::string>(type)))
			continue;

		_hdfFile.readAndResize(std::string("nodes_") + boost::lexical_cast<std::string>(type), allFeatures);

		int dims     = allFeatures.shape(0) - 1;
		int numNodes = allFeatures.shape(1);

		for (int i = 0; i < numNodes; i++) {

			Crag::CragNode n = crag.nodeFromId(allFeatures(0, i));

			std::vector<double> f;
			f.resize(dims);
			std::copy(
					allFeatures.bind<1>(i).begin() + 1,
					allFeatures.bind<1>(i).end(),
					f.begin());
			features.set(n, f);
		}
	}
}

void
Hdf5CragStore::saveEdgeFeatures(const Crag& crag, const EdgeFeatures& features) {

	LOG_USER(hdf5storelog) << "saving edge features... " << std::flush;

	_hdfFile.root();
	_hdfFile.cd_mk("crag");
	_hdfFile.cd_mk("features");

	for (Crag::EdgeType type : Crag::EdgeTypes) {

		int numEdges = 0;
		for (Crag::CragEdge e : crag.edges())
			if (crag.type(e) == type)
				numEdges++;

		if (numEdges == 0)
			continue;

		vigra::MultiArray<2, double> allFeatures(vigra::Shape2(features.dims(type) + 2, numEdges));

		int edgeNum = 0;
		for (Crag::CragEdge e : crag.edges()) {

			if (crag.type(e) != type)
				continue;

			allFeatures(0, edgeNum) = crag.id(e.u());
			allFeatures(1, edgeNum) = crag.id(e.v());
			std::copy(
					features[e].begin(),
					features[e].end(),
					allFeatures.bind<1>(edgeNum).begin() + 2);
			edgeNum++;
		}

		_hdfFile.write(std::string("edges_") + boost::lexical_cast<std::string>(type), allFeatures);
	}

	LOG_USER(hdf5storelog) << "done." << std::endl;
}

void
Hdf5CragStore::retrieveEdgeFeatures(const Crag& crag, EdgeFeatures& features) {

	_hdfFile.root();
	_hdfFile.cd("crag");
	_hdfFile.cd("features");

	for (Crag::EdgeType type : Crag::EdgeTypes) {

		if (!_hdfFile.existsDataset(std::string("edges_") + boost::lexical_cast<std::string>(type)))
			continue;

		vigra::MultiArray<2, double> allFeatures;

		_hdfFile.readAndResize(std::string("edges_") + boost::lexical_cast<std::string>(type), allFeatures);

		int dims     = allFeatures.shape(0) - 2;
		int numEdges = allFeatures.shape(1);

		for (int i = 0; i < numEdges; i++) {

			Crag::CragNode u = crag.nodeFromId(allFeatures(0, i));
			Crag::CragNode v = crag.nodeFromId(allFeatures(1, i));

			auto e = crag.adjEdges(u).begin();
			for (; e != crag.adjEdges(u).end(); e++)
				if ((*e).opposite(u) == v)
					break;

			if (e == crag.adjEdges(u).end())
				UTIL_THROW_EXCEPTION(
						IOError,
						"can not find edge for nodes " << crag.id(u) << " and " << crag.id(v));

			std::vector<double> f;
			f.resize(dims);
			std::copy(
					allFeatures.bind<1>(i).begin() + 2,
					allFeatures.bind<1>(i).end(),
					f.begin());
			features.set(*e, f);
		}
	}
}

void
Hdf5CragStore::saveSkeletons(const Crag& crag, const Skeletons& skeletons) {

	_hdfFile.root();
	_hdfFile.cd_mk("crag");
	_hdfFile.cd_mk("skeletons");

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		int id                   = crag.id(n);
		const Skeleton& skeleton = skeletons[n];
		std::string     name     = boost::lexical_cast<std::string>(id);

		_hdfFile.cd_mk(name);

		writeGraphVolume(skeleton);
		Hdf5GraphWriter::writeNodeMap(skeleton.graph(), skeleton.diameters(), "diameters");

		_hdfFile.cd_up();
	}
}

void
Hdf5CragStore::retrieveSkeletons(const Crag& crag, Skeletons& skeletons) {

	try {

		_hdfFile.cd("/crag/skeletons");

	} catch (vigra::PreconditionViolation& e) {

		return;
	}

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		LOG_ALL(hdf5storelog) << "reading skeleton for node " << crag.id(n) << std::endl;

		std::string name = boost::lexical_cast<std::string>(crag.id(n));

		try {

			_hdfFile.cd(name);

		} catch (vigra::PreconditionViolation& e) {

			continue;
		}

		Skeleton skeleton;
		readGraphVolume(skeleton);
		Hdf5GraphReader::readNodeMap(skeleton.graph(), skeleton.diameters(), "diameters");
		skeletons[n] = std::move(skeleton);

		_hdfFile.cd_up();
	}
}

void
Hdf5CragStore::saveVolumeRays(const VolumeRays& rays) {

	_hdfFile.root();
	_hdfFile.cd_mk("crag");
	_hdfFile.cd_mk("volume_rays");

	for (Crag::CragNode n : rays.getCrag().nodes()) {

		int         id   = rays.getCrag().id(n);
		std::string name = boost::lexical_cast<std::string>(id);

		_hdfFile.cd_mk(name);

		std::vector<double> data;
		for (auto ray : rays[n]) {

			data.push_back(ray.position().x());
			data.push_back(ray.position().y());
			data.push_back(ray.position().z());
			data.push_back(ray.direction().x());
			data.push_back(ray.direction().y());
			data.push_back(ray.direction().z());
		}

		if (data.size() > 0)
			_hdfFile.write(
					"rays",
					vigra::ArrayVectorView<double>(data.size(), const_cast<double*>(&data[0])));

		_hdfFile.cd_up();
	}
}

void
Hdf5CragStore::retrieveVolumeRays(VolumeRays& rays) {

	try {

		_hdfFile.cd("/crag/volume_rays");

	} catch (vigra::PreconditionViolation& e) {

		return;
	}

	for (Crag::CragNode n : rays.getCrag().nodes()) {

		LOG_ALL(hdf5storelog) << "reading volume rays for node " << rays.getCrag().id(n) << std::endl;

		std::string name = boost::lexical_cast<std::string>(rays.getCrag().id(n));

		try {

			_hdfFile.cd(name);

		} catch (vigra::PreconditionViolation& e) {

			continue;
		}

		vigra::ArrayVector<double> data;
		_hdfFile.readAndResize(
				"rays",
				data);

		for (unsigned int i = 0; i < data.size();) {

			util::ray<float, 3> ray;
			ray.position().x() = data[i]; i++;
			ray.position().y() = data[i]; i++;
			ray.position().z() = data[i]; i++;
			ray.direction().x() = data[i]; i++;
			ray.direction().y() = data[i]; i++;
			ray.direction().z() = data[i]; i++;

			rays[n].push_back(ray);
		}

		_hdfFile.cd_up();
	}
}


void
Hdf5CragStore::saveFeatureWeights(const FeatureWeights& weights) {

	_hdfFile.root();
	_hdfFile.cd_mk("/crag");
	writeWeights(weights, "feature_weights");
}

void
Hdf5CragStore::retrieveFeatureWeights(FeatureWeights& weights) {

	_hdfFile.cd("/crag");
	readWeights(weights, "feature_weights");
}

void
Hdf5CragStore::saveFeaturesMin(const FeatureWeights& min) {

	_hdfFile.root();
	_hdfFile.cd_mk("/crag");
	writeWeights(min, "features_min");
}

void
Hdf5CragStore::retrieveFeaturesMin(FeatureWeights& min) {

	_hdfFile.cd("/crag");
	readWeights(min, "features_min");
}

void
Hdf5CragStore::saveFeaturesMax(const FeatureWeights& max) {

	_hdfFile.root();
	_hdfFile.cd_mk("/crag");
	writeWeights(max, "features_max");
}

void
Hdf5CragStore::retrieveFeaturesMax(FeatureWeights& max) {

	_hdfFile.cd("/crag");
	readWeights(max, "features_max");
}

void
Hdf5CragStore::saveCosts(const Crag& crag, const Costs& costs, std::string name) {

	_hdfFile.root();
	_hdfFile.cd_mk("/crag");
	_hdfFile.cd_mk("costs");
	Hdf5GraphWriter::writeNodeMap(crag, costs.node, name + "_nodes");

	std::vector<double> edgeCosts;
	for (Crag::CragEdge e : crag.edges()) {

		edgeCosts.push_back(crag.id(e.u()));
		edgeCosts.push_back(crag.id(e.v()));
		edgeCosts.push_back(costs.edge[e]);
	}
	if (edgeCosts.size() > 0)
		_hdfFile.write(
				name + "_edges",
				vigra::ArrayVectorView<double>(edgeCosts.size(), const_cast<double*>(&edgeCosts[0])));
}

void
Hdf5CragStore::retrieveCosts(const Crag& crag, Costs& costs, std::string name) {

	_hdfFile.cd("/crag");
	_hdfFile.cd("costs");
	Hdf5GraphReader::readNodeMap(crag, costs.node, name + "_nodes");

	if (_hdfFile.existsDataset(name + "_edges")) {

		vigra::ArrayVector<double> edgeCosts;
		_hdfFile.readAndResize(
				name + "_edges",
				edgeCosts);

		for (unsigned int i = 0; i < edgeCosts.size();) {

			Crag::Node u = crag.nodeFromId(edgeCosts[i]);
			Crag::Node v = crag.nodeFromId(edgeCosts[i+1]);
			double cost = edgeCosts[i+2];
			i += 3;

			// find edge in CRAG and set costs
			for (Crag::IncEdgeIt e(crag, u); e != lemon::INVALID; ++e)
				if (crag.getAdjacencyGraph().oppositeNode(u, e) == v) {

					costs.edge[e] = cost;
					break;
				}
		}
	}
}

void
Hdf5CragStore::saveSolution(
		const Crag&         crag,
		const CragSolution& solution,
		std::string         name) {

	_hdfFile.root();
	_hdfFile.cd_mk("solutions");
	_hdfFile.cd_mk(name);

	Crag::NodeMap<int> selectedNodes(crag);
	for (Crag::CragNode n : crag.nodes())
		selectedNodes[n] = solution.selected(n);
	Hdf5GraphWriter::writeNodeMap(crag, selectedNodes, "nodes");

	std::vector<int> selectedEdges;
	for (Crag::CragEdge e : crag.edges()) {

		if (solution.selected(e)) {

			selectedEdges.push_back(crag.id(e.u()));
			selectedEdges.push_back(crag.id(e.v()));
		}
	}
	if (selectedEdges.size() > 0)
		_hdfFile.write(
				"edges",
				vigra::ArrayVectorView<int>(selectedEdges.size(), const_cast<int*>(&selectedEdges[0])));
}

void
Hdf5CragStore::retrieveSolution(
		const Crag&   crag,
		CragSolution& solution,
		std::string   name) {

	_hdfFile.root();
	_hdfFile.cd("solutions");
	_hdfFile.cd(name);

	Crag::NodeMap<int> selectedNodes(crag);
	Hdf5GraphReader::readNodeMap(crag, selectedNodes, "nodes");
	for (Crag::CragNode n : crag.nodes())
		solution.setSelected(n, selectedNodes[n]);

	for (Crag::CragEdge e : crag.edges())
		solution.setSelected(e, false);

	if (!_hdfFile.existsDataset("edges"))
		return;
	vigra::ArrayVector<int> selectedEdges;
	_hdfFile.readAndResize(
			"edges",
			selectedEdges);
	for (unsigned int i = 0; i < selectedEdges.size(); i += 2) {

		Crag::CragNode u = crag.nodeFromId(selectedEdges[i]);
		Crag::CragNode v = crag.nodeFromId(selectedEdges[i+1]);

		// find edge in CRAG
		for (Crag::CragEdge e : crag.adjEdges(u))
			if (e.opposite(u) == v) {

				solution.setSelected(e, true);
				break;
			}
	}
}

std::vector<std::string>
Hdf5CragStore::getSolutionNames() {

	try {

		_hdfFile.root();
		_hdfFile.cd("solutions");

		return _hdfFile.ls();

	} catch (std::exception& e) {

		return std::vector<std::string>();
	}
}

void
Hdf5CragStore::writeGraphVolume(const GraphVolume& graphVolume) {

	PositionConverter positionConverter;

	writeGraph(graphVolume.graph());
	Hdf5GraphWriter::writeNodeMap(graphVolume.graph(), graphVolume.positions(), "positions", positionConverter);

	vigra::MultiArray<1, float> p(3);

	// resolution
	p[0] = graphVolume.getResolutionX();
	p[1] = graphVolume.getResolutionY();
	p[2] = graphVolume.getResolutionZ();
	_hdfFile.write("resolution", p);

	// offset
	p[0] = graphVolume.getOffset().x();
	p[1] = graphVolume.getOffset().y();
	p[2] = graphVolume.getOffset().z();
	_hdfFile.write("offset", p);
}

void
Hdf5CragStore::writeWeights(const FeatureWeights& weights, std::string name) {

	_hdfFile.cd_mk(name);

	for (Crag::NodeType type : Crag::NodeTypes) {

		const std::vector<double>& w = weights[type];

		if (w.size() == 0)
			continue;

		_hdfFile.write(
				std::string("node_") + boost::lexical_cast<std::string>(type),
				vigra::ArrayVectorView<double>(w.size(), const_cast<double*>(&w[0])));
	}

	for (Crag::EdgeType type : Crag::EdgeTypes) {

		const std::vector<double>& w = weights[type];

		if (w.size() == 0)
			continue;

		_hdfFile.write(
				std::string("edge_") + boost::lexical_cast<std::string>(type),
				vigra::ArrayVectorView<double>(w.size(), const_cast<double*>(&w[0])));
	}
}

void
Hdf5CragStore::readWeights(FeatureWeights& weights, std::string name) {

	_hdfFile.cd(name);

	for (Crag::NodeType type : Crag::NodeTypes) {

		if (!_hdfFile.existsDataset(std::string("node_") + boost::lexical_cast<std::string>(type)))
			continue;

		vigra::ArrayVector<double> w;
		_hdfFile.readAndResize(
				std::string("node_") + boost::lexical_cast<std::string>(type),
				w);
		weights[type].resize(w.size());
		std::copy(w.begin(), w.end(), weights[type].begin());
	}

	for (Crag::EdgeType type : Crag::EdgeTypes) {

		if (!_hdfFile.existsDataset(std::string("edge_") + boost::lexical_cast<std::string>(type)))
			continue;

		vigra::ArrayVector<double> w;
		_hdfFile.readAndResize(
				std::string("edge_") + boost::lexical_cast<std::string>(type),
				w);
		weights[type].resize(w.size());
		std::copy(w.begin(), w.end(), weights[type].begin());
	}
}

void
Hdf5CragStore::readGraphVolume(GraphVolume& graphVolume) {

	PositionConverter positionConverter;

	readGraph(graphVolume.graph());
	Hdf5GraphReader::readNodeMap(graphVolume.graph(), graphVolume.positions(), "positions", positionConverter);

	vigra::MultiArray<1, float> p(3);

	// resolution
	_hdfFile.read("resolution", p);
	graphVolume.setResolution(p[0], p[1], p[2]);

	// offset
	_hdfFile.read("offset", p);
	graphVolume.setOffset(p[0], p[1], p[2]);
}
