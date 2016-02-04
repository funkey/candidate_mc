#include <boost/python.hpp>
#include <boost/python/exception_translator.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <util/exceptions.h>
#include <crag/Crag.h>
#include <crag/CragVolumes.h>
#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <io/volumes.h>
#include <inference/Costs.h>
#include <inference/RandomForest.h>
#include <inference/CragSolution.h>
#include <learning/BundleOptimizer.h>
#include <learning/Loss.h>
#include "PyOracle.h"
#include "logging.h"

template <typename Map, typename K, typename V>
const V& genericGetter(const Map& map, const K& k) { return map[k]; }
template <typename Map, typename K, typename V>
void genericSetter(Map& map, const K& k, const V& value) { map[k] = value; }

template <typename Map, typename K, typename V>
void featuresSetter(Map& map, const K& k, const V& value) { map.set(k, value); }

// iterator traits specializations
#if !defined __clang__ || __clang_major__ < 6
namespace boost { namespace detail {
#else
namespace std {
#endif

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

#if !defined __clang__ || __clang_major__ < 6
}} // namespace boost::detail
#else
} // namespace std
#endif

#if defined __clang__ && __clang_major__ < 6
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
			.value("SeparationEdge", Crag::SeparationEdge)
			.value("AssignmentEdge", Crag::AssignmentEdge)
			.value("NoAssignmentEdge", Crag::NoAssignmentEdge)
			;

	// Node, Edge, Arc
	boost::python::class_<Crag::CragNode>("CragNode")
			.def("__eq__", &Crag::CragNode::operator==)
			.def("__ne__", &Crag::CragNode::operator!=)
			;
	boost::python::class_<Crag::CragEdge>("CragEdge", boost::python::init<const Crag&, Crag::RagType::Edge>())
			.def("u", &Crag::CragEdge::u)
			.def("v", &Crag::CragEdge::v)
			;
	boost::python::class_<Crag::CragArc >("CragArc" , boost::python::init<const Crag&, Crag::SubsetType::Arc>())
			.def("source", &Crag::CragArc::source)
			.def("target", &Crag::CragArc::target)
			;

	// iterators
	boost::python::class_<Crag::CragNodes>("CragNodes", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragNodes>())
			.def("__len__", &Crag::CragNodes::size)
			;
	boost::python::class_<Crag::CragEdges>("CragEdges", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragEdges>())
			.def("__len__", &Crag::CragEdges::size)
			;
	boost::python::class_<Crag::CragIncEdges>("CragIncEdges", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragIncEdges>())
			.def("__len__", &Crag::CragIncEdges::size)
			;
	boost::python::class_<Crag::CragArcs>("CragArcs", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragArcs>())
			.def("__len__", &Crag::CragArcs::size)
			;
	boost::python::class_<Crag::CragIncArcs<Crag::InArcTag>>("CragIncInArcs", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragIncArcs<Crag::InArcTag>>())
			.def("__len__", &Crag::CragIncArcs<Crag::InArcTag>::size)
			;
	boost::python::class_<Crag::CragIncArcs<Crag::OutArcTag>>("CragIncOutArcs", boost::python::no_init)
			.def("__iter__", boost::python::iterator<Crag::CragIncArcs<Crag::OutArcTag>>())
			.def("__len__", &Crag::CragIncArcs<Crag::OutArcTag>::size)
			;

	// Crag
	boost::python::class_<Crag, boost::noncopyable>("Crag")
			.def("addNode", static_cast<Crag::CragNode(Crag::*)()>(&Crag::addNode))
			.def("addNode", static_cast<Crag::CragNode(Crag::*)(Crag::NodeType)>(&Crag::addNode))
			.def("addAdjacencyEdge", static_cast<Crag::CragEdge(Crag::*)(Crag::CragNode, Crag::CragNode)>(&Crag::addAdjacencyEdge))
			.def("addAdjacencyEdge", static_cast<Crag::CragEdge(Crag::*)(Crag::CragNode, Crag::CragNode, Crag::EdgeType)>(&Crag::addAdjacencyEdge))
			.def("addSubsetArc", &Crag::addSubsetArc)
			.def("erase", static_cast<void(Crag::*)(Crag::CragNode)>(&Crag::erase))
			.def("erase", static_cast<void(Crag::*)(Crag::CragEdge)>(&Crag::erase))
			.def("erase", static_cast<void(Crag::*)(Crag::CragArc)>(&Crag::erase))
			.def("id", static_cast<int(Crag::*)(Crag::CragNode) const>(&Crag::id))
			.def("id", static_cast<int(Crag::*)(Crag::CragEdge) const>(&Crag::id))
			.def("id", static_cast<int(Crag::*)(Crag::CragArc)  const>(&Crag::id))
			.def("nodeFromId", &Crag::nodeFromId)
			.def("oppositeNode", &Crag::oppositeNode)
			.def("nodes", &Crag::nodes)
			.def("edges", &Crag::edges)
			.def("arcs", &Crag::arcs)
			.def("adjEdges", &Crag::adjEdges)
			.def("outArcs", &Crag::outArcs)
			.def("inArcs", &Crag::inArcs)
			.def("type", static_cast<Crag::NodeType(Crag::*)(Crag::CragNode) const>(&Crag::type))
			.def("type", static_cast<Crag::EdgeType(Crag::*)(Crag::CragEdge) const>(&Crag::type))
			.def("getLevel", &Crag::getLevel)
			.def("isRootNode", &Crag::isRootNode)
			.def("isLeafNode", &Crag::isLeafNode)
			.def("isLeafEdge", &Crag::isLeafEdge)
			.def("leafNodes", &Crag::leafNodes)
			.def("leafEdges", static_cast<std::set<Crag::CragEdge>(Crag::*)(Crag::CragNode) const>(&Crag::leafEdges))
			.def("leafEdges", static_cast<std::set<Crag::CragEdge>(Crag::*)(Crag::CragEdge) const>(&Crag::leafEdges))
			;

	// util::point<float, 3>
	boost::python::class_<util::point<float, 3>>("point_f3")
			.def("x", static_cast<const float&(util::point<float, 3>::*)() const>(&util::point<float, 3>::x),
					boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("y", static_cast<const float&(util::point<float, 3>::*)() const>(&util::point<float, 3>::y),
					boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("z", static_cast<const float&(util::point<float, 3>::*)() const>(&util::point<float, 3>::z),
					boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("__getitem__", &genericGetter<util::point<float,3>, int, float>, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("__setitem__", &genericSetter<util::point<float,3>, int, float>)
			;

	// util::point<int, 3>
	boost::python::class_<util::point<int, 3>>("point_i3")
			.def("x", static_cast<const int&(util::point<int, 3>::*)() const>(&util::point<int, 3>::x),
					boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("y", static_cast<const int&(util::point<int, 3>::*)() const>(&util::point<int, 3>::y),
					boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("z", static_cast<const int&(util::point<int, 3>::*)() const>(&util::point<int, 3>::z),
					boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("__getitem__", &genericGetter<util::point<int,3>, int, int>, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("__setitem__", &genericSetter<util::point<int,3>, int, int>)
			;

	// util::box<T, 3> (aka bounding boxes)
	boost::python::class_<util::box<float, 3>>("box_f3")
			.def("min", static_cast<util::point<float, 3>&(util::box<float, 3>::*)()>(&util::box<float, 3>::min),
					boost::python::return_internal_reference<>())
			.def("max", static_cast<util::point<float, 3>&(util::box<float, 3>::*)()>(&util::box<float, 3>::max),
					boost::python::return_internal_reference<>())
			.def("width", &util::box<float, 3>::width)
			.def("height", &util::box<float, 3>::height)
			.def("depth", &util::box<float, 3>::depth)
			.def("contains", &util::box<float, 3>::template contains<float, 3>)
			;
	boost::python::class_<util::box<int, 3>>("box_i3")
			.def("min", static_cast<util::point<int, 3>&(util::box<int, 3>::*)()>(&util::box<int, 3>::min),
					boost::python::return_internal_reference<>())
			.def("max", static_cast<util::point<int, 3>&(util::box<int, 3>::*)()>(&util::box<int, 3>::max),
					boost::python::return_internal_reference<>())
			.def("width", &util::box<int, 3>::width)
			.def("height", &util::box<int, 3>::height)
			.def("depth", &util::box<int, 3>::depth)
			.def("contains", &util::box<int, 3>::template contains<int, 3>)
			;
	boost::python::class_<util::box<unsigned int, 3>>("box_ui3")
			.def("min", static_cast<util::point<unsigned int, 3>&(util::box<unsigned int, 3>::*)()>(&util::box<unsigned int, 3>::min),
					boost::python::return_internal_reference<>())
			.def("max", static_cast<util::point<unsigned int, 3>&(util::box<unsigned int, 3>::*)()>(&util::box<unsigned int, 3>::max),
					boost::python::return_internal_reference<>())
			.def("width", &util::box<unsigned int, 3>::width)
			.def("height", &util::box<unsigned int, 3>::height)
			.def("depth", &util::box<unsigned int, 3>::depth)
			.def("contains", &util::box<unsigned int, 3>::template contains<unsigned int, 3>)
			.def("contains", &util::box<unsigned int, 3>::template contains<int, 3>)
			;

	// ExplicitVolume<int>
	boost::python::class_<ExplicitVolume<int>>("ExplicitVolume_i")
			.def("getBoundingBox", &ExplicitVolume<int>::getBoundingBox, boost::python::return_internal_reference<>())
			.def("getDiscreteBoundingBox", &ExplicitVolume<int>::getDiscreteBoundingBox, boost::python::return_internal_reference<>())
			.def("getResolution", &ExplicitVolume<int>::getResolution, boost::python::return_internal_reference<>())
			.def("cut", &ExplicitVolume<int>::cut)
			.def("__getitem__", &genericGetter<ExplicitVolume<int>,
					util::point<int,3>, int>, boost::python::return_value_policy<boost::python::copy_const_reference>())
			;

	// CragVolume
	boost::python::class_<CragVolume, std::shared_ptr<CragVolume>>("CragVolume")
			.def("getBoundingBox", &CragVolume::getBoundingBox, boost::python::return_internal_reference<>())
			.def("getDiscreteBoundingBox", &CragVolume::getDiscreteBoundingBox, boost::python::return_internal_reference<>())
			.def("getResolution", &CragVolume::getResolution, boost::python::return_internal_reference<>())
			.def("__getitem__", &genericGetter<ExplicitVolume<unsigned char>, util::point<int,3>, unsigned char>,
					boost::python::return_value_policy<boost::python::copy_const_reference>())
			;

	// volume io
	boost::python::def("readVolume", readVolume<unsigned char>);
	boost::python::def("saveVolume", saveVolume<unsigned char>);
	boost::python::def("readVolume", readVolume<int>);
	boost::python::def("saveVolume", saveVolume<int>);

	// CragVolumes
	boost::python::class_<CragVolumes, boost::noncopyable>("CragVolumes", boost::python::init<const Crag&>())
			.def("setVolume", &CragVolumes::setVolume)
			.def("fillEmptyVolumes", &CragVolumes::fillEmptyVolumes)
			.def("getVolume", &CragVolumes::operator[])
			;

	// node and edge maps
	boost::python::class_<Crag::NodeMap<double>, boost::noncopyable>("CragNodeMap_d", boost::python::init<const Crag&>())
			.def("__getitem__", &genericGetter<Crag::NodeMap<double>, Crag::CragNode, double>, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("__setitem__", &genericSetter<Crag::NodeMap<double>, Crag::CragNode, double>)
			;
	boost::python::class_<Crag::EdgeMap<double>, boost::noncopyable>("CragEdgeMap_d", boost::python::init<const Crag&>())
			.def("__getitem__", &genericGetter<Crag::EdgeMap<double>, Crag::CragEdge, double>, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("__setitem__", &genericSetter<Crag::EdgeMap<double>, Crag::CragEdge, double>)
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

	// std::vector<double>
	boost::python::class_<std::vector<double>>("vector_d")
			.def(boost::python::vector_indexing_suite<std::vector<double>>())
			;

	// std::set<Crag::CragNode>
	boost::python::class_<std::set<Crag::CragNode>>("set_CragNode")
			.def("__iter__", boost::python::iterator<std::set<Crag::CragNode>>())
			.def("__len__", &std::set<Crag::CragNode>::size)
			;

	// std::set<Crag::CragEdge>
	boost::python::class_<std::set<Crag::CragEdge>>("set_CragEdge")
			.def("__iter__", boost::python::iterator<std::set<Crag::CragEdge>>())
			.def("__len__", &std::set<Crag::CragEdge>::size)
			;

	// NodeFeatures
	boost::python::class_<NodeFeatures>("NodeFeatures", boost::python::init<const Crag&>())
			.def("__getitem__", &genericGetter<NodeFeatures, Crag::CragNode, std::vector<double>>,
					boost::python::return_internal_reference<>())
			.def("__setitem__", &featuresSetter<NodeFeatures, Crag::CragNode, std::vector<double>>)
			.def("dims", &NodeFeatures::dims)
			.def("append", &NodeFeatures::append)
			;

	// EdgeFeatures
	boost::python::class_<EdgeFeatures>("EdgeFeatures", boost::python::init<const Crag&>())
			.def("__getitem__", &genericGetter<EdgeFeatures, Crag::CragEdge, std::vector<double>>,
					boost::python::return_internal_reference<>())
			.def("__setitem__", &featuresSetter<EdgeFeatures, Crag::CragEdge, std::vector<double>>)
			.def("dims", &EdgeFeatures::dims)
			.def("append", &EdgeFeatures::append)
			;

	// FeatureWeights
	boost::python::class_<FeatureWeights>("FeatureWeights", boost::python::init<const NodeFeatures&, const EdgeFeatures&, double>())
			.def("__getitem__", &genericGetter<FeatureWeights, Crag::NodeType, std::vector<double>>,
					boost::python::return_internal_reference<>())
			.def("__setitem__", &genericSetter<FeatureWeights, Crag::NodeType, std::vector<double>>)
			.def("__getitem__", &genericGetter<FeatureWeights, Crag::EdgeType, std::vector<double>>,
					boost::python::return_internal_reference<>())
			.def("__setitem__", &genericSetter<FeatureWeights, Crag::EdgeType, std::vector<double>>)
			;

	// CragSolution
	void (CragSolution::*sS1)(Crag::CragNode, bool) = &CragSolution::setSelected;
	void (CragSolution::*sS2)(Crag::CragEdge, bool) = &CragSolution::setSelected;
	bool (CragSolution::*s1)(Crag::CragNode) const  = &CragSolution::selected;
	bool (CragSolution::*s2)(Crag::CragEdge) const  = &CragSolution::selected;
	boost::python::class_<CragSolution, boost::noncopyable>("CragSolution", boost::python::init<const Crag&>())
			.def("setSelected", sS1)
			.def("setSelected", sS2)
			.def("selected", s1)
			.def("selected", s2)
			.def("label", &CragSolution::label)
			;

	// CRAG store
	boost::python::class_<Hdf5CragStore, boost::noncopyable>("Hdf5CragStore", boost::python::init<std::string>())
			.def("saveCrag", &Hdf5CragStore::saveCrag)
			.def("saveVolumes", &Hdf5CragStore::saveVolumes)
			.def("saveNodeFeatures", &Hdf5CragStore::saveNodeFeatures)
			.def("saveEdgeFeatures", &Hdf5CragStore::saveEdgeFeatures)
			//.def("saveSkeletons", &Hdf5CragStore::saveSkeletons)
			.def("saveFeatureWeights", &Hdf5CragStore::saveFeatureWeights)
			//.def("saveFeaturesMin", &Hdf5CragStore::saveFeaturesMin)
			//.def("saveFeaturesMax", &Hdf5CragStore::saveFeaturesMax)
			.def("saveCosts", &Hdf5CragStore::saveCosts)
			.def("saveSolution", &Hdf5CragStore::saveSolution)
			.def("retrieveCrag", &Hdf5CragStore::retrieveCrag)
			.def("retrieveVolumes", &Hdf5CragStore::retrieveVolumes)
			.def("retrieveNodeFeatures", &Hdf5CragStore::retrieveNodeFeatures)
			.def("retrieveEdgeFeatures", &Hdf5CragStore::retrieveEdgeFeatures)
			//.def("retrieveFeaturesMin", &Hdf5CragStore::retrieveFeaturesMin)
			//.def("retrieveFeaturesMax", &Hdf5CragStore::retrieveFeaturesMax)
			//.def("retrieveSkeletons", &Hdf5CragStore::retrieveSkeletons)
			.def("retrieveFeatureWeights", &Hdf5CragStore::retrieveFeatureWeights)
			.def("retrieveCosts", &Hdf5CragStore::retrieveCosts)
			.def("retrieveSolution", &Hdf5CragStore::retrieveSolution)
			.def("getSolutionNames", &Hdf5CragStore::getSolutionNames)
			;

	// volume store
	boost::python::class_<Hdf5VolumeStore, boost::noncopyable>("Hdf5VolumeStore", boost::python::init<std::string>())
			.def("saveIntensities", &Hdf5VolumeStore::saveIntensities)
			.def("saveBoundaries", &Hdf5VolumeStore::saveBoundaries)
			.def("saveGroundTruth", &Hdf5VolumeStore::saveGroundTruth)
			.def("saveLabels", &Hdf5VolumeStore::saveLabels)
			.def("retrieveIntensities", &Hdf5VolumeStore::retrieveIntensities)
			.def("retrieveBoundaries", &Hdf5VolumeStore::retrieveBoundaries)
			.def("retrieveGroundTruth", &Hdf5VolumeStore::retrieveGroundTruth)
			.def("retrieveLabels", &Hdf5VolumeStore::retrieveLabels)
			;

	// RandomForest
	boost::python::class_<RandomForest>("RandomForest")
			.def("prepareTraining", &RandomForest::prepareTraining)
			.def("addSample", &RandomForest::addSample)
			.def("train", &RandomForest::train)
			.def("getProbabilities", &RandomForest::getProbabilities)
			.def("write", &RandomForest::write)
			.def("read", &RandomForest::read)
			.def("getOutOfBagError", &RandomForest::getOutOfBagError)
			.def("getVariableImportance", &RandomForest::getVariableImportance)
			;

	// BundleOptimizer
	boost::python::class_<BundleOptimizer>("BundleOptimizer", boost::python::init<const BundleOptimizer::Parameters&>())
			.def("optimize", &BundleOptimizer::optimize<PyOracle, PyOracle::Weights>)
			.def("getEps", &BundleOptimizer::getEps)
			.def("getMinValue", &BundleOptimizer::getMinValue)
			;

	// BundleOptimizer::Parameters
	boost::python::class_<BundleOptimizer::Parameters>("BundleOptimizerParameters")
			.def_readwrite("lambada" /* "lambda" is a keyword in python */, &BundleOptimizer::Parameters::lambda)
			.def_readwrite("steps", &BundleOptimizer::Parameters::steps)
			.def_readwrite("min_eps", &BundleOptimizer::Parameters::min_eps)
			.def_readwrite("eps_strategy", &BundleOptimizer::Parameters::epsStrategy)
			;

	// BundleOptimizer::OptimizerResult
	boost::python::enum_<BundleOptimizer::OptimizerResult>("BundleOptimizerResult")
			.value("ReachedMinGap", BundleOptimizer::ReachedMinGap)
			.value("ReachedSteps", BundleOptimizer::ReachedSteps)
			.value("Error", BundleOptimizer::Error)
			;

	// BundleOptimizer::EpsStrategy
	boost::python::enum_<BundleOptimizer::EpsStrategy>("BundleOptimizerEpsStrategy")
			.value("EpsFromGap", BundleOptimizer::EpsFromGap)
			.value("EpsFromChange", BundleOptimizer::EpsFromChange)
			;

	// PyOracle
	boost::python::class_<PyOracle>("PyOracle")
			.def("setEvaluateFunctor", &PyOracle::setEvaluateFunctor)
			;

	// PyOracleWeights
	boost::python::class_<PyOracle::Weights>("PyOracleWeights", boost::python::init<std::size_t>())
			.def("__iter__", boost::python::iterator<PyOracle::Weights>())
			.def("__len__", &PyOracle::Weights::size)
			.def("__getitem__", &genericGetter<PyOracle::Weights, size_t, double>, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("__setitem__", &genericSetter<PyOracle::Weights, size_t, double>)
			;
}

} // namespace pycmc
