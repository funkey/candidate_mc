#ifndef HOST_TUBES_IO_HDF5_DIGRAPH_WRITER_H__
#define HOST_TUBES_IO_HDF5_DIGRAPH_WRITER_H__

#include <map>
#include <vigra/hdf5impex.hxx>
#include <lemon/list_graph.h>

class Hdf5DigraphWriter {

public:

	/**
	 * Converts numeric types into array-like objects for HDF5 storage.
	 */
	template <typename T>
	struct DefaultConverter {

		typedef T ArrayValueType;
		static const int ArraySize = 1;

		vigra::ArrayVector<T> operator()(const T& t) const {

			vigra::ArrayVector<T> array(1);
			array[0] = t;
			return array;
		}
	};

	typedef lemon::ListDigraph Digraph;

	template <typename ValueType>
	using NodeMap = Digraph::NodeMap<ValueType>;
	template <typename ValueType>
	using ArcMap = Digraph::ArcMap<ValueType>;

	Hdf5DigraphWriter(vigra::HDF5File& hdfFile) :
		_hdfFile(hdfFile) {}

	/**
	 * Stores the graph structure in the current group as datasets "nodes" and 
	 * "arcs".
	 */
	void writeDigraph(const Digraph& digraph);

	/**
	 * Stores a node map in a dataset with the given name. A converter object 
	 * needs to be provided to transform ValueTypes into ArrayVectorView<T>, 
	 * i.e., memory-consecutive fields of type T. Converter has to define:
	 *
	 *   Converter::ArrayValueType
	 *
	 *     the type T of the ArrayVectorView<T>
	 *
	 *   Converter::ArraySize
	 *
	 *     the number of elements in the array
	 *
	 *   ArrayVectorView<T> operator()(const ValueType& v)
	 *
	 *     the conversion operator
	 */
	template <typename ValueType, typename Converter = DefaultConverter<ValueType>>
	void writeNodeMap(
			const Digraph&            digraph,
			const NodeMap<ValueType>& map,
			std::string               name,
			const Converter&          converter = Converter()) {

		typedef vigra::ArrayVector<typename Converter::ArrayValueType> ArrayType;

		int numNodes = 0;
		for (Digraph::NodeIt node(digraph); node != lemon::INVALID; ++node)
			numNodes++;

		ArrayType values(Converter::ArraySize*numNodes);

		if (!nodeIdsConsequtive(digraph)) {

			std::map<int, int> nodeMap = createNodeMap(digraph);

			for (Digraph::NodeIt node(digraph); node != lemon::INVALID; ++node) {

				ArrayType v = converter(map[node]);
				std::copy(v.begin(), v.end(), values.begin() + Converter::ArraySize*nodeMap[digraph.id(node)]);
			}

		} else {

			for (Digraph::NodeIt node(digraph); node != lemon::INVALID; ++node) {

				ArrayType v = converter(map[node]);
				std::copy(v.begin(), v.end(), values.begin() + Converter::ArraySize*digraph.id(node));
			}
		}

		if (values.size() > 0)
			_hdfFile.write(
					name,
					values);
	}

	/**
	 * Stores an arc map in a dataset with the given name. A converter object 
	 * needs to be provided to transform ValueTypes into ArrayVectorView<T>, 
	 * i.e., memory-consecutive fields of type T. Converter has to define:
	 *
	 *   Converter::ArrayValueType
	 *
	 *     the type T of the ArrayVectorView<T>
	 *
	 *   Converter::ArraySize
	 *
	 *     the number of elements in the array
	 *
	 *   ArrayVectorView<T> operator()(const ValueType& v)
	 *
	 *     the conversion operator
	 */
	template <typename ValueType>
	void writeArcMap(const ArcMap<ValueType>& map, std::string name);

	/**
	 * Stores an arc map with entries of variable length in a dataset with the 
	 * given name. ContainerType has to define:
	 *
	 *   ContainerType::begin(), ContainerType::end()
	 *
	 *     iterators of the collection
	 *
	 *   ContainerType::value_type
	 *
	 *     type of the elements in the collection
	 *
	 *   ContainerType::size()
	 *
	 *     number of elements in the collection
	 *
	 * A converter object needs to be provided to transform 
	 * ContainerType::value_type into ArrayVectorView<T>, i.e., 
	 * memory-consecutive fields of type T. Converter has to define:
	 *
	 *   Converter::ArrayValueType
	 *
	 *     the type T of the ArrayVectorView<T>
	 *
	 *   Converter::ArraySize
	 *
	 *     the number of elements in the array
	 *
	 *   ArrayVectorView<T> operator()(const ValueType& v)
	 *
	 *     the conversion operator
	 */
	template <typename ContainerType, typename Converter>
	void writeVarLengthArcMap(
			const Digraph&                digraph,
			const ArcMap<ContainerType>& map,
			std::string                   name,
			const Converter&              converter) {

		typedef typename ContainerType::value_type                     ValueType;
		typedef vigra::ArrayVector<typename Converter::ArrayValueType> ArrayType;

		ArrayType               values;
		vigra::ArrayVector<int> chunks;

		for (Digraph::ArcIt arc(digraph); arc != lemon::INVALID; ++arc) {

			const ContainerType& arcElements = map[arc];

			chunks.push_back(arcElements.size());
			for (auto& element : arcElements) {

				ArrayType v = converter(element);
				std::copy(v.begin(), v.end(), std::back_inserter(values));
			}
		}

		_hdfFile.write(name + "_values", values);
		_hdfFile.write(name + "_chunks", chunks);
	}

private:

	bool nodeIdsConsequtive(const Hdf5DigraphWriter::Digraph& digraph);

	std::map<int, int> createNodeMap(const Hdf5DigraphWriter::Digraph& digraph);

	vigra::HDF5File& _hdfFile;
};

#endif // HOST_TUBES_IO_HDF5_DIGRAPH_WRITER_H__

