#ifndef HOST_TUBES_IO_HDF5_GRAPH_READER_H__
#define HOST_TUBES_IO_HDF5_GRAPH_READER_H__

#include <vigra/hdf5impex.hxx>
#include <lemon/list_graph.h>

class Hdf5GraphReader {

public:

	/**
	 * Converts numeric types into array-like objects for HDF5 storage.
	 */
	template <typename T, typename To = T>
	struct DefaultConverter {

		typedef T ArrayValueType;
		static const int ArraySize = 1;

		To operator()(const vigra::ArrayVectorView<T>& array) const {

			return static_cast<To>(array[0]);
		}
	};

	typedef lemon::ListGraph Graph;

	template <typename ValueType>
	using NodeMap = Graph::NodeMap<ValueType>;
	template <typename ValueType>
	using EdgeMap = Graph::EdgeMap<ValueType>;

	Hdf5GraphReader(vigra::HDF5File& hdfFile) :
		_hdfFile(hdfFile) {}

	/**
	 * Read the graph structure in the current group with datasets "nodes" and 
	 * "edges".
	 */
	void readGraph(Graph& graph);

	/**
	 * Read a node map from a dataset with the given name. A converter object 
	 * needs to be provided to transform ArrayVectorView<T> objects into 
	 * ValueType. Converter has to define:
	 *
	 *   Converter::ArrayValueType
	 *
	 *     the expected type T of the ArrayVectorView<T>
	 *
	 *   Converter::ArraySize
	 *
	 *     the number of elements in the array
	 *
	 *   ValueType operator()(const ArrayVectorView<T>& a)
	 *
	 *     the conversion operator
	 */
	template <typename ValueType, typename Converter = DefaultConverter<ValueType>>
	void readNodeMap(
			const Graph&        graph,
			NodeMap<ValueType>& map,
			std::string         name,
			const Converter&    converter = Converter()) {

		typedef vigra::ArrayVector<typename Converter::ArrayValueType> ArrayType;

		ArrayType values;

		if (_hdfFile.existsDataset(name))
			_hdfFile.readAndResize(
					name,
					values);

		for (Graph::NodeIt node(graph); node != lemon::INVALID; ++node) {

			std::size_t i = graph.id(node);
			map[node] = converter(values.subarray(i*Converter::ArraySize, (i + 1)*Converter::ArraySize));
		}
	}

private:

	vigra::HDF5File& _hdfFile;
};

#endif // HOST_TUBES_IO_HDF5_GRAPH_READER_H__

