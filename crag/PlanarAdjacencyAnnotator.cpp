#include <util/timing.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include "PlanarAdjacencyAnnotator.h"
#include <vigra/impex.hxx>
#include <vigra/functorexpression.hxx>
#define WITH_LEMON
#include <vigra/multi_gridgraph.hxx>
#include <vigra/adjacency_list_graph.hxx>
#include <vigra/graph_algorithms.hxx>

logger::LogChannel planaradjacencyannotatorlog("planaradjacencyannotatorlog", "[PlanarAdjacencyAnnotator] ");

util::ProgramOption optionCragType(
		util::_long_name        = "cragType",
		util::_description_text = "Controls which candidates are considered for adjacency in the CRAG: "
		                          "'full' adds edges between each adjacent candidate across all levels, "
		                          "'flat' adds edges only between leaf nodes, and 'empty' adds no adjacency "
		                          "edges at all. Default is 'full'.",
		util::_default_value    = "full");

namespace vigra {

	// type traits to use MultiArray as NodeMap
	template <>
	struct GraphMapTypeTraits<MultiArray<3, int> > {
		typedef int        Value;
		typedef int&       Reference;
		typedef const int& ConstReference;
	};
} // namespace vigra

void
PlanarAdjacencyAnnotator::annotate(Crag& crag) {

	if (optionCragType.as<std::string>() == "empty")
		return;

	UTIL_TIME_METHOD;

	util::box<float, 3> cragBB = crag.getBoundingBox();

	util::point<float, 3> resolution;
	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		if (!crag.isLeafNode(n))
			continue;

		resolution = crag.getVolume(n).getResolution();
		break;
	}

	// no nodes?
	if (resolution.isZero())
		return;

	// create a vigra multi-array large enough to hold all volumes
	vigra::MultiArray<3, int> ids(
			vigra::Shape3(
				cragBB.width() /resolution.x(),
				cragBB.height()/resolution.y(),
				cragBB.depth() /resolution.z()),
			std::numeric_limits<int>::max());

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		if (!crag.isLeafNode(n))
			continue;

		const util::point<float, 3>&      volumeOffset     = crag.getVolume(n).getOffset();
		const util::box<unsigned int, 3>& volumeDiscreteBB = crag.getVolume(n).getDiscreteBoundingBox();

		util::point<unsigned int, 3> begin = (volumeOffset - cragBB.min())/resolution;
		util::point<unsigned int, 3> end   = begin +
				util::point<unsigned int, 3>(
						volumeDiscreteBB.width(),
						volumeDiscreteBB.height(),
						volumeDiscreteBB.depth());

		vigra::combineTwoMultiArrays(
				crag.getVolume(n).data(),
				ids.subarray(
						vigra::Shape3(
								begin.x(),
								begin.y(),
								begin.z()),
						vigra::Shape3(
								end.x(),
								end.y(),
								end.z())),
				ids.subarray(
						vigra::Shape3(
								begin.x(),
								begin.y(),
								begin.z()),
						vigra::Shape3(
								end.x(),
								end.y(),
								end.z())),
				vigra::functor::ifThenElse(
						vigra::functor::Arg1() == vigra::functor::Param(1),
						vigra::functor::Param(crag.id(n)),
						vigra::functor::Arg2()
				));
	}

	//vigra::exportImage(
			//ids.bind<2>(0),
			//vigra::ImageExportInfo("debug/ids.tif"));

	typedef vigra::GridGraph<3>       GridGraphType;
	typedef vigra::AdjacencyListGraph RagType;

	GridGraphType grid(
			ids.shape(),
			_neighborhood == Direct ? vigra::DirectNeighborhood : vigra::IndirectNeighborhood);
	RagType rag;
	RagType::EdgeMap<std::vector<GridGraphType::Edge>> affiliatedEdges;

	vigra::makeRegionAdjacencyGraph(
			grid,
			ids,
			rag,
			affiliatedEdges,
			std::numeric_limits<int>::max());

	unsigned int numAdded = 0;
	crag.setGridGraph(grid);
	for (RagType::EdgeIt e(rag); e != lemon::INVALID; ++e) {

		int u = rag.id(rag.u(*e));
		int v = rag.id(rag.v(*e));

		Crag::Edge newEdge = crag.addAdjacencyEdge(
				crag.nodeFromId(u),
				crag.nodeFromId(v));
		crag.setAffiliatedEdges(
				newEdge,
				affiliatedEdges[*e]);
		numAdded++;

		LOG_ALL(planaradjacencyannotatorlog)
				<< "adding leaf node adjacency between "
				<< u << " and " << v << std::endl;
	}

	LOG_USER(planaradjacencyannotatorlog)
			<< "added " << numAdded << " leaf node adjacency edges"
			<< std::endl;

	if (optionCragType.as<std::string>() == "full")
		propagateLeafAdjacencies(crag);
}
