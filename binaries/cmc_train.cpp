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
#include <util/timing.h>
#include <io/CragImport.h>
#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <io/vectors.h>
#include <io/SolutionImageWriter.h>
#include <learning/BestEffort.h>
#include <learning/BundleOptimizer.h>
#include <learning/AssignmentLoss.h>
#include <learning/ContourDistanceLoss.h>
#include <learning/GradientOptimizer.h>
#include <learning/HammingLoss.h>
#include <learning/HausdorffLoss.h>
#include <learning/CragSolverOracle.h>
#include <learning/RandLoss.h>
#include <learning/OverlapLoss.h>
#include <learning/TopologicalLoss.h>

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The treemc project file.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionBestEffortLoss(
		util::_long_name        = "bestEffortLoss",
		util::_description_text = "Use a loss to find the best-effort solution: rand (RAND index approximation "
		                          "to ground-truth), overlap (overlap with ground truth) or hausdorff "
		                          "(minimal Hausdorff distance to any ground-truth region). If not given, the "
		                          "best-effort will be found with a simple heuristic, assigning each leaf candidate "
		                          "to the ground-truth region with maximal overlap.");

util::ProgramOption optionBestEffortFromProjectFile(
		util::_long_name        = "bestEffortFromProjectFile",
		util::_description_text = "Read the best effort solution from the project file.");

util::ProgramOption optionLoss(
		util::_long_name        = "loss",
		util::_description_text = "The loss to use for training: hamming (Hamming distance "
		                          "to best effort, default), rand (RAND index approximation "
		                          "to ground-truth), hausdorff (minimal Hausdorff distance to "
		                          "any ground-truth region), overlap (overlap with ground truth) "
		                          "or topological (penalizes splits, merges, "
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

util::ProgramOption optionRestartTraining(
		util::_long_name        = "restartTraining",
		util::_description_text = "Use the feature weights in the project file as initial weights.");

util::ProgramOption optionNumIterations(
		util::_long_name        = "numIterations",
		util::_description_text = "The number of iterations to spend on finding a solution. Depends on used solver.");

util::ProgramOption optionPretrain(
		util::_long_name        = "pretrain",
		util::_description_text = "Train on a much simpler version of the original problem to get an "
		                          "SVM-like training of the feature weights.");

util::ProgramOption optionOnlyEdgeWeights(
		util::_long_name        = "onlyEdgeWeights",
		util::_description_text = "Train only edge weights.");

util::ProgramOption optionNumSteps(
		util::_long_name        = "numSteps",
		util::_description_text = "The number of steps to perform during training. Defaults to 0, which means no limit.",
		util::_default_value    = 0);

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

util::ProgramOption optionReadOnly(
		util::_long_name        = "readOnly",
		util::_description_text = "Don't write the best-effort or learnt weights to the project file (only export the best-effort).");

util::ProgramOption optionExportBestEffort(
		util::_long_name        = "exportBestEffort",
		util::_description_text = "Create a volume export for the best-effort solution.");

util::ProgramOption optionExportBestEffortWithBoundary(
		util::_long_name        = "exportBestEffortWithBoundary",
		util::_description_text = "Create a volume export for the best-effort solution, showing the boundaries as well.");

int main(int argc, char** argv) {

	UTIL_TIME_SCOPE("main");

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		std::shared_ptr<CragStore> cragStore = std::make_shared<Hdf5CragStore>(optionProjectFile.as<std::string>());
		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());

		LOG_USER(logger::out) << "reading ground-truth" << std::endl;

		ExplicitVolume<int> groundTruth;
		volumeStore.retrieveGroundTruth(groundTruth);

		LOG_USER(logger::out) << "reading CRAG and volumes" << std::endl;

		Crag crag;
		CragVolumes volumes(crag);
		cragStore->retrieveCrag(crag);
		cragStore->retrieveVolumes(volumes);

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		if (!optionDryRun) {

			LOG_USER(logger::out) << "reading features" << std::endl;
			cragStore->retrieveNodeFeatures(crag, nodeFeatures);
			cragStore->retrieveEdgeFeatures(crag, edgeFeatures);
		}

		std::unique_ptr<BestEffort> bestEffort;
		std::unique_ptr<Loss>       bestEffortLoss;
		std::unique_ptr<Loss>       trainingLoss;

		CragSolver::Parameters solverParameters;
		if (optionNumIterations)
			solverParameters.numIterations = optionNumIterations;
		if (optionPretrain)
			solverParameters.noConstraints = true;

		if (optionBestEffortFromProjectFile) {

			LOG_USER(logger::out) << "reading best-effort" << std::endl;

			bestEffort = std::unique_ptr<BestEffort>(new BestEffort(crag));

			cragStore->retrieveSolution(crag, *bestEffort, "best-effort");

		} else {

			if (!optionBestEffortLoss) {

				LOG_USER(logger::out) << "using assignment heuristic for best-effort" << std::endl;

				bestEffort = std::unique_ptr<BestEffort>(new BestEffort(crag, volumes, groundTruth));

			} else {

				if (optionBestEffortLoss.as<std::string>() == "rand") {

					LOG_USER(logger::out) << "using RAND loss for best-effort" << std::endl;

					bestEffortLoss = std::unique_ptr<RandLoss>(new RandLoss(crag, volumes, groundTruth));

				} else if (optionBestEffortLoss.as<std::string>() == "overlap") {

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
			}

			LOG_USER(logger::out) << "storing best-effort solution" << std::endl;

			cragStore->saveSolution(crag, *bestEffort, "best-effort");
		}

		if (optionExportBestEffort) {

			SolutionImageWriter imageWriter;
			imageWriter.setExportArea(groundTruth.getBoundingBox());
			imageWriter.write(crag, volumes, *bestEffort, optionExportBestEffort);
		}

		if (optionExportBestEffortWithBoundary) {

			SolutionImageWriter imageWriter;
			imageWriter.setExportArea(groundTruth.getBoundingBox());
			imageWriter.write(crag, volumes, *bestEffort, optionExportBestEffortWithBoundary, true);
		}

		if (optionLoss.as<std::string>() == "hamming") {

			LOG_USER(logger::out) << "using Hamming loss" << std::endl;

			trainingLoss = std::unique_ptr<HammingLoss>(new HammingLoss(crag, *bestEffort));

		} else if (optionLoss.as<std::string>() == "rand") {

			LOG_USER(logger::out) << "using RAND loss" << std::endl;

			LOG_USER(logger::out) << "reading ground-truth" << std::endl;

			trainingLoss = std::unique_ptr<RandLoss>(new RandLoss(crag, volumes, groundTruth));

		} else if (optionLoss.as<std::string>() == "overlap") {

			LOG_USER(logger::out) << "using overlap loss" << std::endl;

			LOG_USER(logger::out) << "reading ground-truth" << std::endl;
			ExplicitVolume<int> groundTruth;
			volumeStore.retrieveGroundTruth(groundTruth);

			trainingLoss = std::unique_ptr<OverlapLoss>(new OverlapLoss(crag, volumes, groundTruth));

		} else if (optionLoss.as<std::string>() == "hausdorff") {

			LOG_USER(logger::out) << "using hausdorff loss" << std::endl;

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

		// create initial set of weights for the given features
		FeatureWeights weights(nodeFeatures, edgeFeatures, optionInitialWeightValues.as<double>());

		if (optionRestartTraining) {

			FeatureWeights prevWeights;
			cragStore->retrieveFeatureWeights(prevWeights);

			// previous weights might be incomplete
			for (Crag::NodeType type : Crag::NodeTypes)
				if (prevWeights[type].size() > 0)
					std::copy(prevWeights[type].begin(), prevWeights[type].end(), weights[type].begin());
			for (Crag::EdgeType type : Crag::EdgeTypes)
				if (prevWeights[type].size() > 0)
					std::copy(prevWeights[type].begin(), prevWeights[type].end(), weights[type].begin());

			LOG_DEBUG(logger::out) << "starting with feature weights " << weights << std::endl;
		}

		if (optionDryRun) {

			LOG_USER(logger::out) << "dry run -- skip learning" << std::endl;
			if (!optionReadOnly)
				cragStore->saveFeatureWeights(weights);
			return 0;
		}

		CragSolverOracle oracle(
				crag,
				volumes,
				nodeFeatures,
				edgeFeatures,
				*trainingLoss,
				*bestEffort,
				solverParameters);

		UTIL_TIME_SCOPE("training");

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
			parameters.steps       = optionNumSteps;
			BundleOptimizer optimizer(parameters);

			if (optionOnlyEdgeWeights) {

				FeatureWeights mask(weights);
				for (Crag::NodeType type : Crag::NodeTypes)
					std::fill(mask[type].begin(), mask[type].end(), 0);
				for (Crag::EdgeType type : Crag::EdgeTypes)
					std::fill(mask[type].begin(), mask[type].end(), 1);

				optimizer.optimize(oracle, weights, mask);

			} else {

				optimizer.optimize(oracle, weights);
			}
		}

		cragStore->saveFeatureWeights(weights);

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

