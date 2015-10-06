#include <boost/python.hpp>
#include <boost/python/exception_translator.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <util/exceptions.h>
#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <io/Hdf5CragStore.h>
#include <io/volumes.h>
#include <inference/Costs.h>
#include <learning/Loss.h>
#include "logging.h"

template <typename T>
const T& nodeMapGetter(const Crag::NodeMap<T>& map, Crag::CragNode key) { return map[key]; }
template <typename T>
void nodeMapSetter(Crag::NodeMap<T>& map, Crag::CragNode key, const T& value) { map[key] = value; }
template <typename T>
const T& edgeMapGetter(const Crag::EdgeMap<T>& map, Crag::CragEdge key) { return map[key]; }
template <typename T>
void edgeMapSetter(Crag::EdgeMap<T>& map, Crag::CragEdge key, const T& value) { map[key] = value; }

// iterator traits specializations
namespace boost { namespace detail {

template <>
struct iterator_traits<Crag::CragNodeIterator> {

	typedef typename Crag::CragNode value_type;
	typedef typename Crag::CragNode reference;
	typedef int difference_type;
	typedef typename std::forward_iterator_tag iterator_category;
};

template <>
struct iterator_traits<Crag::CragEdgeIterator> {

	typedef typename Crag::CragEdge value_type;
	typedef typename Crag::CragEdge reference;
	typedef int difference_type;
	typedef typename std::forward_iterator_tag iterator_category;
};

template <>
struct iterator_traits<Crag::CragIncEdgeIterator> {

	typedef typename Crag::CragEdge value_type;
	typedef typename Crag::CragEdge reference;
	typedef int difference_type;
	typedef typename std::forward_iterator_tag iterator_category;
};

template <>
struct iterator_traits<Crag::CragArcIterator> {

	typedef typename Crag::CragArc value_type;
	typedef typename Crag::CragArc reference;
	typedef int difference_type;
	typedef typename std::forward_iterator_tag iterator_category;
};

template <typename T>
struct iterator_traits<Crag::CragIncArcIterator<T>> {

	typedef typename Crag::CragArc value_type;
	typedef typename Crag::CragArc reference;
	typedef int difference_type;
	typedef typename std::forward_iterator_tag iterator_category;
};

}} // namespace boost::detail

#ifdef __clang__
// std::shared_ptr support
template<class T> T* get_pointer(std::shared_ptr<T> p){ return p.get(); }
#endif

namespace pycmc {

/**
 * Translates an Exception into a python exception.
 *
 **/
void translateException(const Exception& e) {

	if (boost::get_error_info<error_message>(e))
		PyErr_SetString(PyExc_RuntimeError, boost::get_error_info<error_message>(e)->c_str());
	else
		PyErr_SetString(PyExc_RuntimeError, e.what());
}

/**
 * Defines all the python classes in the module libpycmc. Here we decide 
 * which functions and data members we wish to expose.
 */
BOOST_PYTHON_MODULE(pycmc) {

	boost::python::register_exception_translator<Exception>(&translateException);

	// Logging
	boost::python::enum_<logger::LogLevel>("LogLevel")
			.value("Quiet", logger::Quiet)
			.value("Error", logger::Error)
			.value("Debug", logger::Debug)
			.value("All", logger::All)
			.value("User", logger::User)
			;
	boost::python::def("setLogLevel", setLogLevel);
	boost::python::def("getLogLevel", getLogLevel);

	// NodeType
	boost::python::enum_<Crag::NodeType>("CragNodeType")
			.value("VolumeNode", Crag::VolumeNode)
			.value("SliceNode", Crag::SliceNode)
			.value("AssignmentNode", Crag::AssignmentNode)
			.value("NoAssignmentNode", Crag::NoAssignmentNode)
			;

	// EdgeType
	boost::python::enum_<Crag::EdgeType>("CragEdgeType")
			.value("AdjacencyEdge", Crag::AdjacencyEdge)
			.value("AssignmentEdge", Crag::AssignmentEdge)
			.value("NoAssignmentEdge", Crag::NoAssignmentEdge)
			;

	// Node, Edge, Arc
	boost::python::class_<Crag::CragNode>("CragNode");
	boost::python::class_<Crag::CragEdge>("CragEdge", boost::python::init<const Crag&, Crag::RagType::Edge>());
	boost::python::class_<Crag::CragArc >("CragArc" , boost::python::init<const Crag&, Crag::SubsetType::Arc>());

	// iterators
	boost::python::class_<Crag::CragNodes>("CragNodes", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragNodes>())
			.def("size", &Crag::CragNodes::size)
			;
	boost::python::class_<Crag::CragEdges>("CragEdges", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragEdges>())
			.def("size", &Crag::CragEdges::size)
			;
	boost::python::class_<Crag::CragIncEdges>("CragIncEdges", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragIncEdges>())
			.def("size", &Crag::CragIncEdges::size)
			;
	boost::python::class_<Crag::CragArcs>("CragArcs", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragArcs>())
			.def("size", &Crag::CragArcs::size)
			;
	boost::python::class_<Crag::CragIncArcs<Crag::InArcTag>>("CragIncInArcs", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragIncArcs<Crag::InArcTag>>())
			.def("size", &Crag::CragIncArcs<Crag::InArcTag>::size)
			;
	boost::python::class_<Crag::CragIncArcs<Crag::OutArcTag>>("CragIncOutArcs", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragIncArcs<Crag::OutArcTag>>())
			.def("size", &Crag::CragIncArcs<Crag::OutArcTag>::size)
			;

	// Crag
	boost::python::class_<Crag, boost::noncopyable>("Crag")
			.def("addNode", static_cast<Crag::CragNode(Crag::*)()>(&Crag::addNode))
			.def("addNode", static_cast<Crag::CragNode(Crag::*)(Crag::NodeType)>(&Crag::addNode))
			.def("addAdjacencyEdge", static_cast<Crag::CragEdge(Crag::*)(Crag::CragNode, Crag::CragNode)>(&Crag::addAdjacencyEdge))
			.def("addAdjacencyEdge", static_cast<Crag::CragEdge(Crag::*)(Crag::CragNode, Crag::CragNode, Crag::EdgeType)>(&Crag::addAdjacencyEdge))
			.def("erase", static_cast<void(Crag::*)(Crag::CragNode)>(&Crag::erase))
			.def("erase", static_cast<void(Crag::*)(Crag::CragEdge)>(&Crag::erase))
			.def("erase", static_cast<void(Crag::*)(Crag::CragArc)>(&Crag::erase))
			.def("id", static_cast<int(Crag::*)(Crag::CragNode) const>(&Crag::id))
			.def("id", static_cast<int(Crag::*)(Crag::CragEdge) const>(&Crag::id))
			.def("id", static_cast<int(Crag::*)(Crag::CragArc)  const>(&Crag::id))
			.def("nodeFromId", &Crag::nodeFromId)
			.def("nodes", &Crag::nodes)
			.def("edges", &Crag::edges)
			.def("arcs", &Crag::arcs)
			.def("adjEdges", &Crag::adjEdges)
			.def("outArcs", &Crag::outArcs)
			.def("inArcs", &Crag::inArcs)
			.def("type", static_cast<Crag::NodeType(Crag::*)(Crag::CragNode) const>(&Crag::type))
			.def("type", static_cast<Crag::EdgeType(Crag::*)(Crag::CragEdge) const>(&Crag::type))
			.def("getLevel", &Crag::getLevel)
			;

	// util::point<float, 3>
	boost::python::class_<util::point<float, 3>>("point_f3")
			.def("x", static_cast<const float&(util::point<float, 3>::*)() const>(&util::point<float, 3>::x),
					boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("y", static_cast<const float&(util::point<float, 3>::*)() const>(&util::point<float, 3>::y),
					boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("z", static_cast<const float&(util::point<float, 3>::*)() const>(&util::point<float, 3>::z),
					boost::python::return_value_policy<boost::python::copy_const_reference>())
			;

	// util::box<float, 3> (aka bounding boxes)
	boost::python::class_<util::box<float, 3>>("box_f3")
			.def("min", static_cast<util::point<float, 3>&(util::box<float, 3>::*)()>(&util::box<float, 3>::min),
					boost::python::return_internal_reference<>())
			.def("max", static_cast<util::point<float, 3>&(util::box<float, 3>::*)()>(&util::box<float, 3>::max),
					boost::python::return_internal_reference<>())
			;

	// CragVolume
	boost::python::class_<CragVolume, std::shared_ptr<CragVolume>>("CragVolume")
			.def("getBoundingBox", &CragVolume::getBoundingBox, boost::python::return_internal_reference<>())
			;

	// volume io
	boost::python::def("readVolume", readVolume<unsigned char>);
	boost::python::def("saveVolume", saveVolume<unsigned char>);

	// CragVolumes
	boost::python::class_<CragVolumes, boost::noncopyable>("CragVolumes", boost::python::init<const Crag&>())
			.def("setVolume", &CragVolumes::setVolume)
			.def("fillEmptyVolumes", &CragVolumes::fillEmptyVolumes)
			.def("getVolume", &CragVolumes::operator[])
			;

	// node and edge maps
	boost::python::class_<Crag::NodeMap<double>, boost::noncopyable>("CragNodeMap_d", boost::python::init<const Crag&>())
			.def("__getitem__", &nodeMapGetter<double>, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("__setitem__", &nodeMapSetter<double>)
			;
	boost::python::class_<Crag::EdgeMap<double>, boost::noncopyable>("CragEdgeMap_d", boost::python::init<const Crag&>())
			.def("__getitem__", &edgeMapGetter<double>, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("__setitem__", &edgeMapSetter<double>)
			;

	// Costs
	boost::python::class_<Costs, boost::noncopyable>("Costs", boost::python::init<const Crag&>())
			.def_readonly("node", &Costs::node)
			.def_readonly("edge", &Costs::edge)
			;

	// Loss (need to expose 'node' and 'edge' again?)
	boost::python::class_<Loss, boost::python::bases<Costs>, boost::noncopyable>("Loss", boost::python::init<const Crag&>())
			.def_readwrite("constant", &Loss::constant)
			;

	// CRAG stores
	boost::python::class_<Hdf5CragStore, boost::noncopyable>("Hdf5CragStore", boost::python::init<std::string>())
			.def("saveCrag", &Hdf5CragStore::saveCrag)
			.def("saveVolumes", &Hdf5CragStore::saveVolumes)
			//.def("saveNodeFeatures", &Hdf5CragStore::saveNodeFeatures)
			//.def("saveEdgeFeatures", &Hdf5CragStore::saveEdgeFeatures)
			//.def("saveSkeletons", &Hdf5CragStore::saveSkeletons)
			//.def("saveFeatureWeights", &Hdf5CragStore::saveFeatureWeights)
			//.def("saveFeaturesMin", &Hdf5CragStore::saveFeaturesMin)
			//.def("saveFeaturesMax", &Hdf5CragStore::saveFeaturesMax)
			.def("saveCosts", &Hdf5CragStore::saveCosts)
			//.def("saveSolution", &Hdf5CragStore::saveSolution)
			.def("retrieveCrag", &Hdf5CragStore::retrieveCrag)
			.def("retrieveVolumes", &Hdf5CragStore::retrieveVolumes)
			//.def("retrieveNodeFeatures", &Hdf5CragStore::retrieveNodeFeatures)
			//.def("retrieveEdgeFeatures", &Hdf5CragStore::retrieveEdgeFeatures)
			//.def("retrieveFeaturesMin", &Hdf5CragStore::retrieveFeaturesMin)
			//.def("retrieveFeaturesMax", &Hdf5CragStore::retrieveFeaturesMax)
			//.def("retrieveSkeletons", &Hdf5CragStore::retrieveSkeletons)
			//.def("retrieveFeatureWeights", &Hdf5CragStore::retrieveFeatureWeights)
			.def("retrieveCosts", &Hdf5CragStore::retrieveCosts)
			//.def("retrieveSolution", &Hdf5CragStore::retrieveSolution)
			.def("getSolutionNames", &Hdf5CragStore::getSolutionNames)
			;
}

} // namespace pycmc
