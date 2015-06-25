#include <fstream>
#include <util/Logger.h>
#include "volumes.h"

void readCrag(std::string filename, Crag& crag, util::point<float, 3> resolution, util::point<float, 3> offset) {

	vigra::ImageImportInfo info(filename.c_str());
	Image mergeTree(info.width(), info.height());
	importImage(info, vigra::destImage(mergeTree));
	mergeTree.setResolution(resolution);
	mergeTree.setOffset(offset);

	MergeTreeParser parser(mergeTree);
	parser.getCrag(crag);

	PlanarAdjacencyAnnotator annotator(PlanarAdjacencyAnnotator::Direct);
	annotator.annotate(crag);
}

void readCrag(std::string superpixels, std::string mergeHistory, Crag& crag, util::point<float, 3> resolution, util::point<float, 3> offset) {

	ExplicitVolume<int> ids = readVolume<int>(getImageFiles(superpixels));

	int _, maxId;
	ids.data().minmax(&_, &maxId);

	LOG_USER(logger::out) << "sopervoxels stack contains ids between " << _ << " and " << maxId << std::endl;

	std::map<int, util::box<int, 3>> bbs;
	for (unsigned int z = 0; z < ids.depth();  z++)
	for (unsigned int y = 0; y < ids.height(); y++)
	for (unsigned int x = 0; x < ids.width();  x++) {

		int id = ids(x, y, z);
		bbs[id].fit(
				util::box<int, 3>(
						x,   y,   z,
						x+1, y+1, z+1));
	}

	std::map<int, Crag::Node> idToNode;
	for (const auto& p : bbs) {

		const int& id               = p.first;
		const util::box<int, 3>& bb = p.second;

		Crag::Node n = crag.addNode();
		crag.getVolumeMap()[n] = ExplicitVolume<unsigned char>(bb.width(), bb.height(), bb.depth(), 0);
		crag.getVolumeMap()[n].setResolution(resolution);
		crag.getVolumeMap()[n].setOffset(offset + bb.min()*resolution);
		idToNode[id] = n;
	}

	for (unsigned int z = 0; z < ids.depth();  z++)
	for (unsigned int y = 0; y < ids.height(); y++)
	for (unsigned int x = 0; x < ids.width();  x++) {

		int id = ids(x, y, z);
		Crag::Node n = idToNode[id];

		crag.getVolumeMap()[n](x - bbs[id].min().x(), y - bbs[id].min().y(), z - bbs[id].min().z()) = 1;
	}

	std::ifstream file(mergeHistory);
	if (file.fail()) {

		LOG_ERROR(logger::out) << "could not read merge history" << std::endl;
		return;
	}

	while (true) {
		int a, b, c;
		file >> a;
		file >> b;
		file >> c;
		if (!file.good())
			break;

		Crag::Node n = crag.addNode();
		idToNode[c] = n;

		if (!idToNode.count(a))
			std::cerr << "node " << a << " is used for merging, but was not encountered before" << std::endl;
		if (!idToNode.count(b))
			std::cerr << "node " << b << " is used for merging, but was not encountered before" << std::endl;

		crag.addSubsetArc(
				idToNode[a],
				n);
		crag.addSubsetArc(
				idToNode[b],
				n);
	}

	PlanarAdjacencyAnnotator annotator(PlanarAdjacencyAnnotator::Direct);
	annotator.annotate(crag);
}

std::vector<std::string>
getImageFiles(std::string path) {

	std::vector<std::string> filenames;

	boost::filesystem::path p(path);

	if (boost::filesystem::is_directory(p)) {

		for (boost::filesystem::directory_iterator i(p); i != boost::filesystem::directory_iterator(); i++)
			if (!boost::filesystem::is_directory(*i) && (
			    i->path().extension() == ".png" ||
			    i->path().extension() == ".tif" ||
			    i->path().extension() == ".tiff"
			))
				filenames.push_back(i->path().native());

		std::sort(filenames.begin(), filenames.end());

	} else {

		filenames.push_back(path);
	}

	return filenames;
}
