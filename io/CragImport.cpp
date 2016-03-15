#include <fstream>
#include <limits>
#include <vigra/impex.hxx>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <crag/MergeTreeParser.h>
#include "CragImport.h"
#include "volumes.h"

util::ProgramOption optionMaxMerges(
		util::_long_name        = "maxMerges",
		util::_description_text = "The maximal depth of the CRAG subset tree, starting counting from the leaf nodes.");

util::ProgramOption optionMergeHistoryWithScores(
		util::_long_name        = "mergeHistoryWithScores",
		util::_description_text = "Indicate that the merge history file contains lines with 'a b c score' for merges of a and b into c.");

util::ProgramOption optionMaxMergeScore(
		util::_long_name        = "maxMergeScore",
		util::_description_text = "The maximal score of a merge to add to the CRAG. Scores are read from the merge history file.");

util::ProgramOption option2dSupervoxels(
		util::_long_name        = "2dSupervoxels",
		util::_description_text = "Indicate that all supervoxels are 2D slices (even though the volume is 3D). This will create "
		                          "a CRAG with SliceNodes instead of VolumeNodes. SliceNodes have more features that only apply "
		                          "to 2D objects.");

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
CragImport::readCragFromMergeHistory(
		std::string           supervoxels,
		std::string           mergeHistory,
		Crag&                 crag,
		CragVolumes&          volumes,
		util::point<float, 3> resolution,
		util::point<float, 3> offset,
		Costs&                mergeCosts) {

	ExplicitVolume<int> ids = readVolume<int>(getImageFiles(supervoxels));

	bool is2D = false;
	if (ids.depth() == 1 || option2dSupervoxels)
		is2D = true;

	std::map<int, Crag::Node> idToNode = readSupervoxels(ids, crag, volumes, resolution, offset);

	int maxMerges = -1;
	if (optionMaxMerges)
		maxMerges = optionMaxMerges;

	bool useScores = optionMergeHistoryWithScores.as<bool>();

	std::ifstream file(mergeHistory);
	if (file.fail()) {

		LOG_ERROR(logger::out) << "could not read merge history" << std::endl;
		return;
	}

	double maxScore = std::numeric_limits<double>::max();
	if (optionMaxMergeScore)
		maxScore = optionMaxMergeScore.as<double>();

	while (true) {
		int a, b, c;
		file >> a;
		file >> b;
		file >> c;
		double score = 0;
		if (useScores)
			file >> score;

		if (!file.good())
			break;

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

		Crag::Node n = crag.addNode(is2D ? Crag::SliceNode : Crag::VolumeNode);
		idToNode[c] = n;
		if (useScores)
			mergeCosts.node[n] = score;

		if (!idToNode.count(a))
			std::cerr << "node " << a << " is used for merging, but was not encountered before" << std::endl;
		if (!idToNode.count(b))
			std::cerr << "node " << b << " is used for merging, but was not encountered before" << std::endl;

		LOG_ALL(logger::out) << "merging " << a << " and " << b << " to " << c << std::endl;

		crag.addSubsetArc(
				idToNode[a],
				n);
		crag.addSubsetArc(
				idToNode[b],
				n);
	}

	volumes.fillEmptyVolumes();

	if (option2dSupervoxels) {

		for (Crag::CragNode n : crag.nodes()) {

			const CragVolume& volume = *volumes[n];

			if (volume.depth() != 1)
				UTIL_THROW_EXCEPTION(
						UsageError,
						"option '2dSupervoxels' was given, but after import, CRAG contains a node with depth " << volume.depth() << ". Check if the initial supervoxels are really 2D, and that the merge history only merges in 2D.");
		}
	}
}

void
CragImport::readCragFromCandidateSegmentation(
		std::string           supervoxels,
		std::string           candidateSegmentation,
		Crag&                 crag,
		CragVolumes&          volumes,
		util::point<float, 3> resolution,
		util::point<float, 3> offset) {

	ExplicitVolume<int> ids = readVolume<int>(getImageFiles(supervoxels));

	bool is2D = false;
	if (ids.depth() == 1 || option2dSupervoxels)
		is2D = true;

	std::map<int, Crag::Node> svIdToNode = readSupervoxels(ids, crag, volumes, resolution, offset);

	LOG_USER(logger::out) << "reading segmentation" << std::endl;

	ExplicitVolume<int> segmentation = readVolume<int>(getImageFiles(candidateSegmentation));

	// get all segments
	std::set<int> segmentIds;
	for (int id : segmentation.data())
		if (id != 0)
			segmentIds.insert(id);

	LOG_USER(logger::out) << "found " << segmentIds.size() << " segments" << std::endl;
	LOG_USER(logger::out) << "assigning supervoxels to segments" << std::endl;

	// get overlap of each supervoxel with segments
	std::map<int /*sv id*/, std::map<int /*segment id*/, int /*size*/>> overlap;
	for (unsigned int z = 0; z < ids.depth(); z++)
	for (unsigned int y = 0; y < ids.height(); y++)
	for (unsigned int x = 0; x < ids.width(); x++) {

		int svId  = ids(x, y, z);
		int segId = segmentation(x, y, z);

		if (segId != 0)
			overlap[svId][segId]++;
	}

	// create a node for each segment (that has overlapping supervoxels) and 
	// link to max-overlap nodes
	std::map<int, Crag::Node> segIdToNode;
	for (const auto& nodeSegOverlap : overlap) {

		int svId = nodeSegOverlap.first;
		int maxOverlap = 0;
		int maxSegmentId = 0;

		for (const auto& segOverlap : nodeSegOverlap.second) {

			int segId = segOverlap.first;
			int overlap = segOverlap.second;

			if (segId != 0 && overlap >= maxOverlap) {

				maxOverlap = overlap;
				maxSegmentId = segId;
			}
		}

		if (!segIdToNode.count(maxSegmentId)) {

			Crag::Node candidate = crag.addNode(is2D ? Crag::SliceNode : Crag::VolumeNode);
			segIdToNode[maxSegmentId] = candidate;
		}

		crag.addSubsetArc(svIdToNode[svId], segIdToNode[maxSegmentId]);
	}

	volumes.fillEmptyVolumes();
}

std::map<int, Crag::Node>
CragImport::readSupervoxels(
		const ExplicitVolume<int>& ids,
		Crag&                      crag,
		CragVolumes&               volumes,
		util::point<float, 3>      resolution,
		util::point<float, 3>      offset) {

	bool is2D = false;
	if (ids.depth() == 1 || option2dSupervoxels)
		is2D = true;

	int minId, maxId;
	ids.data().minmax(&minId, &maxId);

	LOG_USER(logger::out) << "sopervoxels stack contains ids between " << minId << " and " << maxId << std::endl;

	std::map<int, util::box<int, 3>> bbs;
	for (unsigned int z = 0; z < ids.depth();  z++)
	for (unsigned int y = 0; y < ids.height(); y++)
	for (unsigned int x = 0; x < ids.width();  x++) {

		int id = ids(x, y, z);

		if (id == 0)
			continue;

		bbs[id].fit(
				util::box<int, 3>(
						x,   y,   z,
						x+1, y+1, z+1));
	}

	std::map<int, Crag::Node> idToNode;
	for (const auto& p : bbs) {

		const int& id               = p.first;
		const util::box<int, 3>& bb = p.second;

		Crag::Node n = crag.addNode(is2D ? Crag::SliceNode : Crag::VolumeNode);
		std::shared_ptr<CragVolume> volume = std::make_shared<CragVolume>(bb.width(), bb.height(), bb.depth(), 0);
		volume->setResolution(resolution);
		volume->setOffset(offset + bb.min()*resolution);
		volumes.setVolume(n, volume);
		idToNode[id] = n;
	}
	volumes.fillEmptyVolumes();

	for (unsigned int z = 0; z < ids.depth();  z++)
	for (unsigned int y = 0; y < ids.height(); y++)
	for (unsigned int x = 0; x < ids.width();  x++) {

		int id = ids(x, y, z);

		if (id == 0)
			continue;

		Crag::Node n = idToNode[id];

		(*volumes[n])(x - bbs[id].min().x(), y - bbs[id].min().y(), z - bbs[id].min().z()) = 1;
	}

	return idToNode;
}

