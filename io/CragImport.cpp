#include <fstream>
#include <vigra/impex.hxx>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <crag/MergeTreeParser.h>
#include "CragImport.h"
#include "volumes.h"

util::ProgramOption optionMaxMerges(
		util::_long_name        = "maxMerges",
		util::_description_text = "The maximal depth of the CRAG subset tree, starting counting from the leaf nodes.");

util::ProgramOption optionMaxMergeScore(
		util::_long_name        = "maxMergeScore",
		util::_description_text = "The maximal score of a merge to add to the CRAG. Scores are read from file mergeScores.");

void
CragImport::readCrag(
		std::string           filename,
		Crag&                 crag,
		CragVolumes&          volumes,
		util::point<float, 3> resolution,
		util::point<float, 3> offset) {

	int maxMerges = -1;
	if (optionMaxMerges)
		maxMerges = optionMaxMerges;

	vigra::ImageImportInfo info(filename.c_str());
	Image mergeTree(info.width(), info.height());
	importImage(info, vigra::destImage(mergeTree));
	mergeTree.setResolution(resolution);
	mergeTree.setOffset(offset);

	MergeTreeParser parser(mergeTree, maxMerges);
	parser.getCrag(crag, volumes);
}

void
CragImport::readCrag(
		std::string           superpixels,
		std::string           mergeHistory,
		std::string           mergeScores,
		Crag&                 crag,
		CragVolumes&          volumes,
		util::point<float, 3> resolution,
		util::point<float, 3> offset) {

	std::map<int, Crag::Node> idToNode = readSupervoxels(crag, volumes, resolution, offset, superpixels);

	int maxMerges = -1;
	if (optionMaxMerges)
		maxMerges = optionMaxMerges;

	std::ifstream file(mergeHistory);
	if (file.fail()) {

		LOG_ERROR(logger::out) << "could not read merge history" << std::endl;
		return;
	}

	std::ifstream scores(mergeScores);
	if (optionMaxMerges && scores.fail()) {

		LOG_ERROR(logger::out) << "could not read merge scores" << std::endl;
		return;
	}

	bool useScores = (mergeScores.size() > 0) && optionMaxMergeScore;
	double maxScore = optionMaxMergeScore.as<double>();

	while (true) {
		int a, b, c;
		file >> a;
		file >> b;
		file >> c;
		if (!file.good())
			break;

		double score = 0;
		if (useScores)
			scores >> score;

		// we might encounter ids that we didn't add, since they are too high in 
		// the merge tree or have a score exceeding maxScore
		if (!idToNode.count(a))
			continue;
		if (!idToNode.count(b))
			continue;

		// are we limiting the number of merges?
		if (maxMerges >= 0) {

			if (crag.getLevel(idToNode[a]) >= maxMerges)
				continue;
			if (crag.getLevel(idToNode[b]) >= maxMerges)
				continue;
		}

		// are we limiting the merge score?
		if (useScores && score >= maxScore)
			continue;

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
}

void
CragImport::readCrag(
		std::string           superpixels,
		std::string           candidateSegmentation,
		Crag&                 crag,
		CragVolumes&          volumes,
		util::point<float, 3> resolution,
		util::point<float, 3> offset) {

	std::map<int, Crag::Node> svIdToNode = readSupervoxels(crag, volumes, resolution, offset, superpixels);

	LOG_USER(logger::out) << "reading segmentation" << std::endl;

	ExplicitVolume<int> segmentation = readVolume<int>(getImageFiles(candidateSegmentation));

	LOG_USER(logger::out) << "grouping supervoxels" << std::endl;

	// get all segments
	std::set<int> segmentIds;
	for (int id : segmentation.data())
		segmentIds.insert(id);

	// get overlap of each node with segments
	std::map<Crag::Node, std::map<int, int>> overlap;
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		// Here, we assume that the given volumes (supervoxels and candidate 
		// segmentation) have the same resolution.

		//if (segmentation.getResolution() != crag.getVolume(n).getResolution())
			//UTIL_THROW_EXCEPTION(
					//UsageError,
					//"candidate segmentation has different resolution (" << segmentation.getResolution() <<
					//" than supervoxels " << crag.getVolume(n).getResolution());

		util::point<int, 3> offset = volumes[n]->getOffset()/volumes[n]->getResolution() - segmentation.getOffset();

		const util::box<int, 3>& bb = volumes[n]->getDiscreteBoundingBox();
		for (int z = 0; z < bb.depth();  z++)
		for (int y = 0; y < bb.height(); y++)
		for (int x = 0; x < bb.width();  x++) {

			int segId = segmentation(
					offset.x() + x,
					offset.y() + y,
					offset.z() + z);

			overlap[n][segId]++;
		}
	}

	// create a node for each segment (that has overlapping supervoxels) and 
	// link to max-overlap nodes
	std::map<int, Crag::Node> segIdToNode;
	for (const auto& nodeSegOverlap : overlap) {

		Crag::Node leaf = nodeSegOverlap.first;
		int maxOverlap = 0;
		int maxSegmentId = 0;

		for (const auto& segOverlap : nodeSegOverlap.second) {

			if (segOverlap.first != 0 && segOverlap.second >= maxOverlap) {

				maxOverlap = segOverlap.second;
				maxSegmentId = segOverlap.first;
			}
		}

		if (!segIdToNode.count(maxSegmentId)) {

			Crag::Node candidate = crag.addNode();
			segIdToNode[maxSegmentId] = candidate;
		}

		crag.addSubsetArc(leaf, segIdToNode[maxSegmentId]);
	}
}

std::map<int, Crag::Node>
CragImport::readSupervoxels(
		Crag&                 crag,
		CragVolumes&          volumes,
		util::point<float, 3> resolution,
		util::point<float, 3> offset,
		std::string           supervoxels) {

	ExplicitVolume<int> ids = readVolume<int>(getImageFiles(supervoxels));

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
		std::shared_ptr<CragVolume> volume = std::make_shared<CragVolume>(bb.width(), bb.height(), bb.depth(), 0);
		volume->setResolution(resolution);
		volume->setOffset(offset + bb.min()*resolution);
		volumes.setLeafNodeVolume(n, volume);
		idToNode[id] = n;
	}

	for (unsigned int z = 0; z < ids.depth();  z++)
	for (unsigned int y = 0; y < ids.height(); y++)
	for (unsigned int x = 0; x < ids.width();  x++) {

		int id = ids(x, y, z);
		Crag::Node n = idToNode[id];

		(*volumes[n])(x - bbs[id].min().x(), y - bbs[id].min().y(), z - bbs[id].min().z()) = 1;
	}

	return idToNode;
}

