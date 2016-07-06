#include <util/Logger.h>
#include "MergeTreeParser.h"

logger::LogChannel mergetreeparserlog("mergetreeparserlog", "[MergeTreeParser] ");

util::ProgramOption optionSpacedEdgeImage(
		util::_long_name        = "spacedEdgeImage",
		util::_description_text = "Indicate that the merge-tree image is 4x the original image size, "
		                          "with boundary values stored between the pixels. This is used to "
		                          "create regions without boundary pixels between them.");

MergeTreeParser::MergeTreeParser(
		const Image& mergeTree,
		int maxMerges,
		unsigned int minRegionSize,
		unsigned int maxRegionSize) :
	_mergeTree(mergeTree),
	_maxMerges(maxMerges),
	_minRegionSize(minRegionSize),
	_maxRegionSize(maxRegionSize) {}

void
MergeTreeParser::getCrag(Crag& crag, CragVolumes& volumes) {

	ImageParser::Parameters parameters;

	parameters.darkToBright    = true;
	parameters.spacedEdgeImage = optionSpacedEdgeImage;

	ImageParser parser(_mergeTree, parameters);

	MergeTreeVisitor visitor(
			_mergeTree.getResolution(),
			_mergeTree.getBoundingBox().min(),
			crag,
			volumes,
			_maxMerges,
			_minRegionSize,
			_maxRegionSize);

	parser.parse(visitor);
}

MergeTreeParser::MergeTreeVisitor::MergeTreeVisitor(
		const util::point<float, 3>& resolution,
		const util::point<float, 3>& offset,
		Crag&                        crag,
		CragVolumes&                 volumes,
		int                          maxMerges,
		unsigned int                 minRegionSize,
		unsigned int                 maxRegionSize) :
	_resolution(resolution),
	_offset(offset),
	_crag(crag),
	_volumes(volumes),
	_minSize(minRegionSize),
	_maxSize(maxRegionSize),
	_extents(_crag),
	_maxMerges(maxMerges) {}

void
MergeTreeParser::MergeTreeVisitor::finalizeComponent(
		float                        /*value*/,
		PixelList::const_iterator    begin,
		PixelList::const_iterator    end) {

	bool changed = (begin != _prevBegin || end != _prevEnd);

	_prevBegin = begin;
	_prevEnd   = end;

	if (!changed)
		return;

	LOG_ALL(mergetreeparserlog) << "found a new component" << std::endl;

	size_t size = end - begin;
	bool validSize  = (size >= _minSize && (_maxSize == 0 || size < _maxSize));

	if (!validSize)
		return;

	// get all prospective children of this component

	std::vector<Crag::CragNode> children;
	int level = 0;
	while (!_roots.empty() && contained(_extents[_roots.top()], std::make_pair(begin, end))) {

		children.push_back(_roots.top());
		level = std::max(level, _crag.getLevel(_roots.top()) + 1);
		_roots.pop();
	}

	if (_maxMerges >= 0 && level > _maxMerges) {

		for (auto c = children.rbegin(); c != children.rend(); c++)
			_roots.push(*c);
		return;
	}

	LOG_ALL(mergetreeparserlog) << "add it to crag" << std::endl;

	// create a node (all nodes from a 2D merge-tree are slice nodes)
	Crag::CragNode node = _crag.addNode(Crag::SliceNode);
	_extents[node] = std::make_pair(begin, end);

	// connect it to children
	for (Crag::CragNode child : children)
		_crag.addSubsetArc(child, node);

	bool isLeafNode = (level == 0);
	LOG_ALL(mergetreeparserlog) << "is" << (isLeafNode ? "" : " not") << " a leaf node" << std::endl;

	// extract and set volume

	util::box<unsigned int, 3> boundingBox;

	for (PixelList::const_iterator i = begin; i != end; i++)
		boundingBox.fit(
				util::box<unsigned int, 3>(
						util::point<unsigned int, 3>(
								i->x(),     i->y(),     0),
						util::point<unsigned int, 3>(
								i->x() + 1, i->y() + 1, 1)));

	std::shared_ptr<CragVolume> volume = std::make_shared<CragVolume>(
			boundingBox.width(),
			boundingBox.height(),
			boundingBox.depth(),
			false);

	for (PixelList::const_iterator i = begin; i != end; i++)
		(*volume)[i->project<3>() - boundingBox.min()] = true;

	util::point<float, 3> volumeOffset = _offset + boundingBox.min()*_resolution;

	volume->setResolution(_resolution);
	volume->setOffset(volumeOffset);
	_volumes.setVolume(node, volume);

	// put the new node on the stack
	_roots.push(node);
}
