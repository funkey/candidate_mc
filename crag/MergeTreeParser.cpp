#include <util/Logger.h>
#include "MergeTreeParser.h"

logger::LogChannel mergetreeparserlog("mergetreeparserlog", "[MergeTreeParser] ");

util::ProgramOption optionMinRegionSize(
		util::_long_name        = "minRegionSize",
		util::_description_text = "The minimal size of a region in pixels.",
		util::_default_value    = 0);

util::ProgramOption optionMaxRegionSize(
		util::_long_name        = "maxSliceSize",
		util::_description_text = "The maximal size of a region in pixels.",
		util::_default_value    = 10e5);

util::ProgramOption optionSpacedEdgeImage(
		util::_long_name        = "spacedEdgeImage",
		util::_description_text = "Indicate that the merge-tree image is 4x the original image size, "
		                          "with boundary values stored between the pixels. This is used to "
		                          "create regions without boundary pixels between them.");

MergeTreeParser::MergeTreeParser(const Image& mergeTree) :
	_mergeTree(mergeTree) {}

void
MergeTreeParser::getCrag(Crag& crag) {

	ImageParser::Parameters parameters;

	parameters.darkToBright    = true;
	parameters.spacedEdgeImage = optionSpacedEdgeImage;

	ImageParser parser(_mergeTree, parameters);

	MergeTreeVisitor visitor(
			(optionSpacedEdgeImage ? _mergeTree.getResolution()/2 : _mergeTree.getResolution()),
			_mergeTree.getBoundingBox().min(),
			crag);

	parser.parse(visitor);
}

MergeTreeParser::MergeTreeVisitor::MergeTreeVisitor(
		const util::point<float, 3>& resolution,
		const util::point<float, 3>& offset,
		Crag& crag) :
	_resolution(resolution),
	_offset(offset),
	_crag(crag),
	_minSize(optionMinRegionSize),
	_maxSize(optionMaxRegionSize),
	_extents(_crag),
	_isLeafeNode(_crag) {}

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

	LOG_ALL(mergetreeparserlog) << "add it to crag" << std::endl;

	// create a node
	Crag::Node node = _crag.addNode();
	_extents[node] = std::make_pair(begin, end);

	bool isLeafNode = true;

	// make all open root nodes that are subsets children of this component
	while (!_roots.empty() && contained(_extents[_roots.top()], std::make_pair(begin, end))) {

		_crag.addSubsetArc(_roots.top(), node);
		_roots.pop();
		isLeafNode = false;
	}

	LOG_ALL(mergetreeparserlog) << "is" << (isLeafNode ? "" : " not") << " a leaf node" << std::endl;

	// extract volumes for leaf nodes
	if (isLeafNode) {

		util::box<unsigned int, 3> boundingBox;

		for (PixelList::const_iterator i = begin; i != end; i++)
			boundingBox.fit(
					util::point<unsigned int, 3>(
							i->x(), i->y(), 0));
		boundingBox.max().x() += 1;
		boundingBox.max().y() += 1;
		boundingBox.max().z() += 1;

		ExplicitVolume<unsigned char> volume(
				boundingBox.width(),
				boundingBox.height(),
				boundingBox.depth(),
				false);

		for (PixelList::const_iterator i = begin; i != end; i++)
			volume[i->project<3>() - boundingBox.min()] = true;

		util::point<float, 3> volumeOffset = _offset + boundingBox.min()*_resolution;

		volume.setResolution(_resolution);
		volume.setOffset(volumeOffset);
		_crag.getVolumes()[node] = std::move(volume);
	}

	_isLeafeNode[node] = isLeafNode;

	// put the new node on the stack
	_roots.push(node);
}
