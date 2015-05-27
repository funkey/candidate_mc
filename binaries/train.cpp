/**
 * Reads a treemc project file containing features and a ground-truth labelling 
 * and trains node and edge feature weights.
 */

#include <iostream>
#include <boost/filesystem.hpp>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <io/Hdf5CragStore.h>
#include <learning/BundleOptimizer.h>
#include <learning/Oracle.h>

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The treemc project file.",
		util::_default_value    = "project.hdf");

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		Hdf5CragStore cragStore(optionProjectFile.as<std::string>());

		Crag crag;
		cragStore.retrieveCrag(crag);

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		cragStore.retrieveNodeFeatures(crag, nodeFeatures);
		cragStore.retrieveEdgeFeatures(crag, edgeFeatures);

		BundleOptimizer::Parameters parameters;
		parameters.epsStrategy = BundleOptimizer::EpsFromGap;
		BundleOptimizer optimizer(parameters);

		// TODO:
		// • replace with real loss and best effort
		Loss       loss(crag);
		BestEffort bestEffort(crag);

		Crag::NodeIt n(crag);
		bestEffort.node[n] = true;

		Oracle oracle(
				crag,
				nodeFeatures,
				edgeFeatures,
				loss,
				bestEffort);

		std::vector<double> weights(nodeFeatures.dims() + edgeFeatures.dims(), 0);
		optimizer.optimize(oracle, weights);

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}


