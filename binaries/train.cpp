/**
 * Reads a treemc project file containing features and a ground-truth labelling 
 * and trains node and edge feature weights.
 */

#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <learning/BundleOptimizer.h>
#include <learning/Oracle.h>
#include <learning/OverlapLoss.h>
#include <learning/HammingLoss.h>
#include <learning/BestEffort.h>

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The treemc project file.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionFeatureWeights(
		util::_long_name        = "featureWeights",
		util::_short_name       = "w",
		util::_description_text = "A file to store the learnt feature weights.",
		util::_default_value    = "feature_weights.txt");

util::ProgramOption optionLoss(
		util::_long_name        = "loss",
		util::_description_text = "The loss to use for training: hamming (Hamming distance "
		                          "to best effort, default) or overlap (RAND index approximation "
		                          "to ground-truth).",
		util::_default_value    = "hamming");


util::ProgramOption optionNormalizeLoss(
		util::_long_name        = "normalizeLoss",
		util::_description_text = "Normalize the loss, such that values on valid solutions are in [0,1].");

util::ProgramOption optionRegularizerWeight(
		util::_long_name        = "regularizerWeight",
		util::_description_text = "The factor of the quadratic regularizer on w.",
		util::_default_value    = 1.0);

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		Hdf5CragStore   cragStore(optionProjectFile.as<std::string>());
		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());

		Crag crag;
		cragStore.retrieveCrag(crag);

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		cragStore.retrieveNodeFeatures(crag, nodeFeatures);
		cragStore.retrieveEdgeFeatures(crag, edgeFeatures);

		BundleOptimizer::Parameters parameters;
		parameters.lambda      = optionRegularizerWeight;
		parameters.epsStrategy = BundleOptimizer::EpsFromGap;
		BundleOptimizer optimizer(parameters);

		ExplicitVolume<int> groundTruth;
		volumeStore.retrieveGroundTruth(groundTruth);
		OverlapLoss overlapLoss(crag, groundTruth);
		BestEffort  bestEffort(crag, overlapLoss);

		Loss* loss = 0;
		bool  destructLoss = false;

		if (optionLoss.as<std::string>() == "hamming") {

			loss = new HammingLoss(crag, bestEffort);
			destructLoss = true;

		} else if (optionLoss.as<std::string>() == "overlap") {

			loss = &overlapLoss;

		} else {

			LOG_ERROR(logger::out)
					<< "unknown loss: "
					<< optionLoss.as<std::string>()
					<< std::endl;
			return 1;
		}

		if (optionNormalizeLoss)
			loss->normalize(crag, MultiCut::Parameters());

		Oracle oracle(
				crag,
				nodeFeatures,
				edgeFeatures,
				*loss,
				bestEffort);

		std::vector<double> weights(nodeFeatures.dims() + edgeFeatures.dims(), 0);
		optimizer.optimize(oracle, weights);

		std::ofstream weightsFile(optionFeatureWeights.as<std::string>());
		for (double f : weights)
			weightsFile << f << std::endl;

		if (destructLoss && loss != 0)
			delete loss;

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}


