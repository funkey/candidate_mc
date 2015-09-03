#ifndef CANDIDATE_MC_CRAG_MERGE_TREE_PARSER_H__
#define CANDIDATE_MC_CRAG_MERGE_TREE_PARSER_H__

#include <vigra/multi_array.hxx>
#include <util/ProgramOptions.h>
#include <imageprocessing/Image.h>
#include <imageprocessing/PixelList.h>
#include <imageprocessing/ImageLevelParser.h>
#include "Crag.h"
#include "CragVolumes.h"

class MergeTreeParser {

public:

	/**
	 * Create a new MergeTreeParser for a merge tree image.
	 *
	 * @param mergeTree
	 *              The merge tree image.
	 * @param maxMerges
	 *              Don't extract candidates that have a level higher than 
	 *              maxMerges. Set it to -1 to disable.
	 */
	MergeTreeParser(const Image& mergeTree, int maxMerges = -1);

	/**
	 * Get the candidate region adjacency graph from the given merge tree image. 
	 * Note that this only extracts the subset relations, not the adjacency 
	 * (since the latter might be application dependent).
	 *
	 * @param[out] crag
	 *                   An empty CRAG to fill.
	 * @param[out] volumes
	 *                   A node map to store the leaf node volumes.
	 */
	void getCrag(Crag& crag, CragVolumes& volumes);

private:

	typedef ImageLevelParser<unsigned short> ImageParser;

	class MergeTreeVisitor : public ImageParser::Visitor {

	public:

		/**
		 * Create a merge tree visitor. The visitor will create ExplicitVolumes 
		 * for each leaf node.
		 *
		 * @param resolution
		 *              The resolution of the merge tree image.
		 * @param offset
		 *              The offset of the merge tree image.
		 */
		MergeTreeVisitor(
				const util::point<float, 3>& resolution,
				const util::point<float, 3>& offset,
				Crag&                        crag,
				CragVolumes&                 volumes,
				int                          maxMerges);

		/**
		 * Set the pixel list that contains the pixel locations of each 
		 * component. The iterators passed by finalizeComponent refer to indices 
		 * in this pixel list.
		 *
		 * @param pixelList
		 *              A pixel list shared between all components.
		 */
		void setPixelList(boost::shared_ptr<PixelList> pixelList) { _pixelList = pixelList; }

		/**
		 * Invoked whenever the current component was extracted entirely.  
		 * Indicates that we go up by one level in the component tree and make 
		 * the parent of the current component the new current component.
		 *
		 * @param value
		 *              The threshold value of the current component.
		 *
		 * @param begin, end
		 *              Iterators into the pixel list that define the pixels of 
		 *              the current component.
		 */
		void finalizeComponent(
				Image::value_type            value,
				PixelList::const_iterator    begin,
				PixelList::const_iterator    end);

	private:

		// is the first range contained in the second?
		inline bool contained(
				const std::pair<PixelList::const_iterator, PixelList::const_iterator>& a,
				const std::pair<PixelList::const_iterator, PixelList::const_iterator>& b) {

			return (a.first >= b.first && a.second <= b.second);
		}

		util::point<float, 3> _resolution;
		util::point<float, 3> _offset;

		Crag&        _crag;
		CragVolumes& _volumes;

		unsigned int _minSize;
		unsigned int _maxSize;

		boost::shared_ptr<PixelList> _pixelList;

		// extents of the previous component to detect changes
		PixelList::const_iterator _prevBegin;
		PixelList::const_iterator _prevEnd;

		// stack of open root nodes while constructing the tree
		std::stack<Crag::CragNode> _roots;

		// extents of all regions
		Crag::NodeMap<std::pair<PixelList::const_iterator, PixelList::const_iterator>> _extents;

		int _maxMerges;
	};

	const Image& _mergeTree;

	int _maxMerges;
};

#endif // CANDIDATE_MC_CRAG_MERGE_TREE_PARSER_H__

