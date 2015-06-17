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

	} else {

		filenames.push_back(path);
	}

	return filenames;
}
