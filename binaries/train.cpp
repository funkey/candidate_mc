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
#include <io/CragImport.h>
#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <io/vectors.h>
#include <learning/BestEffort.h>
#include <learning/BundleOptimizer.h>
#include <learning/AssignmentLoss.h>
#include <learning/ContourDistanceLoss.h>
#include <learning/GradientOptimizer.h>
#include <learning/HammingLoss.h>
#include <learning/HausdorffLoss.h>
#include <learning/Oracle.h>
#include <learning/OverlapLoss.h>
#include <learning/TopologicalLoss.h>

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The treemc project file.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionBestEffortLoss(
		util::_long_name        = "bestEffortLoss",
		util::_description_text = "The loss to use for finding the best-effort solution: overlap (RAND index approximation "
		                          "to ground-truth, default) or hausdorff (minimal Hausdorff distance to any ground-truth region).",
		util::_default_value    = "overlap");

util::ProgramOption optionBestEffortFromProjectFile(
		util::_long_name        = "bestEffortFromProjectFile",
		util::_description_text = "Read the best effort solution from the project file.");

util::ProgramOption optionLoss(
		util::_long_name        = "loss",
		util::_description_text = "The loss to use for training: hamming (Hamming distance "
		                          "to best effort, default), overlap (RAND index approximation "
		                          "to ground-truth), hausdorff (minimal Hausdorff distance to "
		                          "any ground-truth region), or topological (penalizes splits, merges, "
		                          "false positives and false negatives). Any other value will try to "
		                          "find a loss function with this name in the training dataset.",
		util::_default_value    = "hamming");

util::ProgramOption optionNormalizeLoss(
		util::_long_name        = "normalizeLoss",
		util::_description_text = "Normalize the loss, such that values on valid solutions are in [0,1].");

util::ProgramOption optionRegularizerWeight(
		util::_long_name        = "regularizerWeight",
		util::_description_text = "The factor of the quadratic regularizer on w.",
		util::_default_value    = 1.0);

util::ProgramOption optionInitialWeightValues(
		util::_long_name        = "initialWeightValues",
		util::_description_text = "Uniform values of the weight vectors to start learning with.",
		util::_default_value    = 0);

util::ProgramOption optionInitialWeights(
		util::_long_name        = "initialWeights",
		util::_description_text = "A file containing an initial set of weights.");

util::ProgramOption optionPretrain(
		util::_long_name        = "pretrain",
		util::_description_text = "Train on a much simpler version of the original problem to get an "
		                          "SVM-like training of the feature weights.");

util::ProgramOption optionGradientOptimizer(
		util::_long_name        = "gradientOptimizer",
		util::_description_text = "Use a simple gradient descent to minimize the training objective.");

util::ProgramOption optionInitialStepWidth(
		util::_long_name        = "initialStepWidth",
		util::_description_text = "Initial step width for the gradient optimizer.",
		util::_default_value    = 1.0);

util::ProgramOption optionMaxHausdorffDistance(
		util::_module           = "loss.hausdorff",
		util::_long_name        = "maxDistance",
		util::_description_text = "The maximal Hausdorff distance that will be used for the Hausdorff loss.",
		util::_default_value    = 1000); // matching tobasl experiments

util::ProgramOption optionDryRun(
		util::_long_name        = "dryRun",
		util::_description_text = "Compute and store the best-effort loss, the best-effort, and the training loss; but "
		                          "don't perform training.");

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		std::shared_ptr<CragStore> cragStore = std::make_shared<Hdf5CragStore>(optionProjectFile.as<std::string>());
		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());

		Crag crag;
		CragVolumes volumes(crag);
		cragStore->retrieveCrag(crag);
		cragStore->retrieveVolumes(volumes);

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		LOG_USER(logger::out) << "reading features" << std::endl;
		cragStore->retrieveNodeFeatures(crag, nodeFeatures);
		cragStore->retrieveEdgeFeatures(crag, edgeFeatures);

		std::unique_ptr<BestEffort> bestEffort;
		std::unique_ptr<Loss>       bestEffortLoss;
		std::unique_ptr<Loss>       trainingLoss;

		CragSolver::Parameters solverParameters;
		if (optionPretrain)
			solverParameters.noConstraints = true;

		if (optionBestEffortFromProjectFile) {

			LOG_USER(logger::out) << "reading best-effort" << std::endl;

			bestEffort = std::unique_ptr<BestEffort>(new BestEffort(crag));

			cragStore->retrieveSolution(crag, *bestEffort, "best-effort");

		} else {

			LOG_USER(logger::out) << "reading ground-truth" << std::endl;
			ExplicitVolume<int> groundTruth;
			volumeStore.retrieveGroundTruth(groundTruth);

			if (optionBestEffortLoss.as<std::string>() == "overlap") {

				LOG_USER(logger::out) << "using overlap loss for best-effort" << std::endl;

				bestEffortLoss = std::unique_ptr<OverlapLoss>(new OverlapLoss(crag, volumes, groundTruth));

			} else if (optionBestEffortLoss.as<std::string>() == "hausdorff") {

				LOG_USER(logger::out) << "using hausdorff loss for best-effort" << std::endl;

				// get ground truth volumes
				Crag        gtCrag;
				CragVolumes gtVolumes(gtCrag);
				CragImport  import;
				import.readSupervoxels(groundTruth, gtCrag, gtVolumes, groundTruth.getResolution(), groundTruth.getOffset());

				bestEffortLoss = std::unique_ptr<HausdorffLoss>(new HausdorffLoss(crag, volumes, gtCrag, gtVolumes, optionMaxHausdorffDistance));

			} else if (optionBestEffortLoss.as<std::string>() == "contour") {

				LOG_USER(logger::out) << "using contour loss for best-effort" << std::endl;

				// get ground truth volumes
				Crag        gtCrag;
				CragVolumes gtVolumes(gtCrag);
				CragImport  import;
				import.readSupervoxels(groundTruth, gtCrag, gtVolumes, groundTruth.getResolution(), groundTruth.getOffset());

				bestEffortLoss = std::unique_ptr<ContourDistanceLoss>(new ContourDistanceLoss(crag, volumes, gtCrag, gtVolumes, optionMaxHausdorffDistance));

			} else if (optionBestEffortLoss.as<std::string>() == "assignment") {

				LOG_USER(logger::out) << "using assignment loss for best-effort" << std::endl;

				bestEffortLoss = std::unique_ptr<AssignmentLoss>(new AssignmentLoss(crag, volumes, groundTruth));

			} else {

				UTIL_THROW_EXCEPTION(
						UsageError,
						"unknown best-effort loss " + optionBestEffortLoss.as<std::string>());
			}

			LOG_USER(logger::out) << "storing best-effort loss" << std::endl;

			cragStore->saveCosts(crag, *bestEffortLoss, "best-effort_loss");

			LOG_USER(logger::out) << "finding best-effort solution" << std::endl;

			bestEffort = std::unique_ptr<BestEffort>(new BestEffort(crag, volumes, *bestEffortLoss, solverParameters));

			LOG_USER(logger::out) << "storing best-effort solution" << std::endl;

			cragStore->saveSolution(crag, *bestEffort, "best-effort");
		}

		if (optionLoss.as<std::string>() == "hamming") {

			LOG_USER(logger::out) << "using Hamming loss" << std::endl;

			trainingLoss = std::unique_ptr<HammingLoss>(new HammingLoss(crag, *bestEffort));

		} else if (optionLoss.as<std::string>() == "overlap") {

			LOG_USER(logger::out) << "using overlap loss" << std::endl;

			LOG_USER(logger::out) << "reading ground-truth" << std::endl;
			ExplicitVolume<int> groundTruth;
			volumeStore.retrieveGroundTruth(groundTruth);

			trainingLoss = std::unique_ptr<OverlapLoss>(new OverlapLoss(crag, volumes, groundTruth));

		} else if (optionLoss.as<std::string>() == "hausdorff") {

			LOG_USER(logger::out) << "using hausdorff loss" << std::endl;

			ExplicitVolume<int> groundTruth;
			volumeStore.retrieveGroundTruth(groundTruth);

			// get ground truth volumes
			Crag        gtCrag;
			CragVolumes gtVolumes(gtCrag);
			CragImport  import;
			import.readSupervoxels(groundTruth, gtCrag, gtVolumes, groundTruth.getResolution(), groundTruth.getOffset());

			trainingLoss = std::unique_ptr<HausdorffLoss>(new HausdorffLoss(crag, volumes, gtCrag, gtVolumes, optionMaxHausdorffDistance));

		} else if (optionLoss.as<std::string>() == "topological") {

			LOG_USER(logger::out) << "using topological loss" << std::endl;

			trainingLoss = std::unique_ptr<TopologicalLoss>(new TopologicalLoss(crag, *bestEffort));

		} else {

			LOG_USER(logger::out) << "using custom loss " << optionLoss.as<std::string>() << std::endl;

			trainingLoss = std::unique_ptr<Loss>(new Loss(crag));
			cragStore->retrieveCosts(crag, *trainingLoss, optionLoss);
		}

		if (optionNormalizeLoss) {

			LOG_USER(logger::out) << "normalizing loss..." << std::endl;
			trainingLoss->normalize(crag, solverParameters);
		}

		LOG_USER(logger::out) << "storing training loss" << std::endl;

		cragStore->saveCosts(crag, *trainingLoss, "training_loss");

		if (optionDryRun) {

			LOG_USER(logger::out) << "dry run -- skip learning" << std::endl;
			return 0;
		}

		Oracle oracle(
				crag,
				volumes,
				nodeFeatures,
				edgeFeatures,
				*trainingLoss,
				*bestEffort,
				solverParameters);

		// create initial set of weights for the given features
		FeatureWeights weights(nodeFeatures, edgeFeatures, optionInitialWeightValues.as<double>());

		if (optionGradientOptimizer) {

			GradientOptimizer::Parameters parameters;
			parameters.lambda           = optionRegularizerWeight.as<double>();
			parameters.initialStepWidth = optionInitialStepWidth.as<double>();
			GradientOptimizer optimizer(parameters);
			optimizer.optimize(oracle, weights);

		} else {

			BundleOptimizer::Parameters parameters;
			parameters.lambda      = optionRegularizerWeight;
			parameters.epsStrategy = BundleOptimizer::EpsFromGap;
			BundleOptimizer optimizer(cragStore, parameters);
			optimizer.optimize(oracle, weights);
		}

		cragStore->saveFeatureWeights(weights);

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

