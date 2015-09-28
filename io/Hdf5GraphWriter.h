#ifndef HOST_TUBES_IO_HDF5_GRAPH_WRITER_H__
#define HOST_TUBES_IO_HDF5_GRAPH_WRITER_H__

#include <map>
#include <vigra/hdf5impex.hxx>
#include <lemon/list_graph.h>

class Hdf5GraphWriter {

public:

	/**
	 * Converts numeric types into array-like objects for HDF5 storage.
	 */
	template <typename T, typename To = T>
	struct DefaultConverter {

		typedef To ArrayValueType;
		static const int ArraySize = 1;

		vigra::ArrayVector<To> operator()(const T& t) const {

			vigra::ArrayVector<To> array(1);
			array[0] = t;
			return array;
		}
	};

	typedef lemon::ListGraph Graph;

	template <typename ValueType>
	using NodeMap = Graph::NodeMap<ValueType>;
	template <typename ValueType>
	using EdgeMap = Graph::EdgeMap<ValueType>;

	Hdf5GraphWriter(vigra::HDF5File& hdfFile) :
		_hdfFile(hdfFile) {}

	/**
	 * Stores the graph structure in the current group as datasets "nodes" and 
	 * "edges".
	 */
	void writeGraph(const Graph& graph);

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
			const Graph&              graph,
			const NodeMap<ValueType>& map,
			std::string               name,
			const Converter&          converter = Converter()) {

		typedef vigra::ArrayVector<typename Converter::ArrayValueType> ArrayType;

		int numNodes = 0;
		for (Graph::NodeIt node(graph); node != lemon::INVALID; ++node)
			numNodes++;

		ArrayType values(Converter::ArraySize*numNodes);

		if (!nodeIdsConsequtive(graph)) {

			std::map<int, int> nodeMap = createNodeMap(graph);

			for (Graph::NodeIt node(graph); node != lemon::INVALID; ++node) {

				ArrayType v = converter(map[node]);
				std::copy(v.begin(), v.end(), values.begin() + Converter::ArraySize*nodeMap[graph.id(node)]);
			}

		} else {

			for (Graph::NodeIt node(graph); node != lemon::INVALID; ++node) {

				ArrayType v = converter(map[node]);
				std::copy(v.begin(), v.end(), values.begin() + Converter::ArraySize*graph.id(node));
			}
		}

		if (values.size() > 0)
			_hdfFile.write(
					name,
					values);
	}


private:

	bool nodeIdsConsequtive(const Hdf5GraphWriter::Graph& graph);

	std::map<int, int> createNodeMap(const Hdf5GraphWriter::Graph& graph);

	vigra::HDF5File& _hdfFile;
};

#endif // HOST_TUBES_IO_HDF5_GRAPH_WRITER_H__

