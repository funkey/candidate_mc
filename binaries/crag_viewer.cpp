/**
 * This programs visualizes a CRAG stored in an hdf5 file.
 */

#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <util/ProgramOptions.h>
#include <util/string.h>
#include <imageprocessing/ExplicitVolume.h>
#include <gui/CragView.h>
#include <gui/MeshViewController.h>
#include <gui/CostsView.h>
#include <gui/FeaturesView.h>
#include <gui/SolutionView.h>
#include <sg_gui/RotateView.h>
#include <sg_gui/ZoomView.h>
#include <sg_gui/Window.h>

using namespace sg_gui;

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The project file to read the CRAG from.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionIntensities(
		util::_long_name        = "intensities",
		util::_description_text = "Which volume to show as intensities. 'raw' shows the raw intensity volume, 'boundary' "
		                          "the boundary prediction volume. Default is 'raw'.",
		util::_default_value    = "raw");

util::ProgramOption optionOverlay(
		util::_long_name        = "overlay",
		util::_description_text = "The type of labels to show as overlay on the volume. 'leaf' shows the CRAG leaf nodes, "
		                          "any other string connected components of a solution with that name in the project file. "
		                          "Default is 'leaf'. Multiple overlays can be given by separating them with commas. They can "
		                          "be switched between with the number keys.",
		util::_default_value    = "leaf");

util::ProgramOption optionCandidates(
		util::_long_name        = "candidates",
		util::_description_text = "The candidates to show as meshes when double-clicking on the volume. 'crag' shows the candidates of the CRAG, "
		                          "any other string connected components of a solution with that name in the project file. "
		                          "Default is 'crag'.",
		util::_default_value    = "crag");

util::ProgramOption optionShowCosts(
		util::_long_name        = "showCosts",
		util::_description_text = "For each selected candidate, show the costs with the given name (default: 'costs', the inference costs).",
		util::_default_value    = "costs");

util::ProgramOption optionShowFeatures(
		util::_long_name        = "showFeatures",
		util::_description_text = "For each selected candidate, show the features.");

util::ProgramOption optionShowSolution(
		util::_long_name        = "showSolution",
		util::_description_text = "For each selected candidate, show whether it is part of the solution with the given name.");

std::shared_ptr<ExplicitVolume<float>>
getOverlay(
		std::string name,
		const Crag& crag,
		const CragVolumes& volumes,
		CragStore& cragStore,
		VolumeStore& volumeStore,
		std::shared_ptr<ExplicitVolume<float>> supervoxels) {

	std::shared_ptr<ExplicitVolume<float>> overlay;

	if (name == "leaf") {

		LOG_USER(logger::out) << "showing CRAG leaf node labels in overlay" << std::endl;

		overlay = supervoxels;

	} else {

		LOG_USER(logger::out) << "showing " << name << " labels in overlay" << std::endl;

		try {

			ExplicitVolume<int> tmp;
			volumeStore.retrieveVolume(tmp, name);

			overlay = std::make_shared<ExplicitVolume<float>>();
			*overlay = tmp;

		} catch (std::exception e) {

			LOG_USER(logger::out) << "did not find volume with name " << name << std::endl;
			LOG_USER(logger::out) << "looking for a solution with that name" << std::endl;

			std::shared_ptr<CragSolution> overlaySolution = std::make_shared<CragSolution>(crag);
			cragStore.retrieveSolution(crag, *overlaySolution, name);

			overlay =
					std::make_shared<ExplicitVolume<float>>(
							supervoxels->width(),
							supervoxels->height(),
							supervoxels->depth());
			overlay->setResolution(supervoxels->getResolution());
			overlay->setOffset(supervoxels->getOffset());

			for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

				if (!overlaySolution->selected(n))
					continue;

				const CragVolume& volume = *volumes[n];
				util::point<int, 3> offset = volume.getOffset()/volume.getResolution();

				for (unsigned int z = 0; z < volume.getDiscreteBoundingBox().depth();  z++)
				for (unsigned int y = 0; y < volume.getDiscreteBoundingBox().height(); y++)
				for (unsigned int x = 0; x < volume.getDiscreteBoundingBox().width();  x++) {

					if (volume.data()(x, y, z))
						(*overlay)(
								offset.x() + x,
								offset.y() + y,
								offset.z() + z)
										= overlaySolution->label(n) + 1;
				}
			}
		}
	}

	return overlay;
}

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		// create hdf5 stores

		Hdf5CragStore   cragStore(optionProjectFile.as<std::string>());
		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());

		// get crag and volumes

		Crag         crag;
		CragVolumes  volumes(crag);

		try {

			cragStore.retrieveCrag(crag);
			cragStore.retrieveVolumes(volumes);

		} catch (std::exception& e) {

			LOG_USER(logger::out) << "could not find a CRAG" << std::endl;
		}

		// get intensities and supervoxel labels

		auto intensities = std::make_shared<ExplicitVolume<float>>();
		if (optionIntensities.as<std::string>() == "raw")
			volumeStore.retrieveIntensities(*intensities);
		else
			volumeStore.retrieveBoundaries(*intensities);
		intensities->normalize();
		float min, max;
		intensities->data().minmax(&min, &max);

		auto supervoxels =
				std::make_shared<ExplicitVolume<float>>(
						intensities->width(),
						intensities->height(),
						intensities->depth());
		supervoxels->setResolution(intensities->getResolution());
		supervoxels->setOffset(intensities->getOffset());
		supervoxels->data() = 0;

		for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

			if (!crag.isLeafNode(n))
				continue;

			const CragVolume& volume = *volumes[n];
			util::point<int, 3> offset = volume.getOffset()/volume.getResolution();

			for (unsigned int z = 0; z < volume.getDiscreteBoundingBox().depth();  z++)
			for (unsigned int y = 0; y < volume.getDiscreteBoundingBox().height(); y++)
			for (unsigned int x = 0; x < volume.getDiscreteBoundingBox().width();  x++) {

				if (volume.data()(x, y, z))
					(*supervoxels)(
							offset.x() + x,
							offset.y() + y,
							offset.z() + z)
									= crag.id(n);
			}
		}

		std::vector<std::shared_ptr<ExplicitVolume<float>>> overlays;
		std::shared_ptr<CragSolution>          overlaySolution;
		std::shared_ptr<CragSolution>          viewSolution;

		for (std::string name : split(optionOverlay, ','))
			overlays.push_back(getOverlay(name, crag, volumes, cragStore, volumeStore, supervoxels));

		if (optionShowSolution) {

			viewSolution = std::make_shared<CragSolution>(crag);
			cragStore.retrieveSolution(crag, *viewSolution, optionShowSolution.as<std::string>());
		}

		bool showSolutionCandidates = false;
		if (optionCandidates.as<std::string>() != "crag") {

			if (!overlaySolution) {

				overlaySolution = std::make_shared<CragSolution>(crag);
				cragStore.retrieveSolution(crag, *overlaySolution, optionCandidates.as<std::string>());
			}

			showSolutionCandidates = true;
		}

		// get features

		NodeFeatures nodeFeatures(crag);
		EdgeFeatures edgeFeatures(crag);

		try {

			LOG_USER(logger::out) << "reading features..." << std::flush;

			cragStore.retrieveNodeFeatures(crag, nodeFeatures);
			cragStore.retrieveEdgeFeatures(crag, edgeFeatures);

			LOG_USER(logger::out) << "done." << std::endl;

		} catch (std::exception& e) {

			LOG_USER(logger::out) << "could not find features" << std::endl;
		}

		// get costs

		Costs costs(crag);

		try {

			cragStore.retrieveCosts(crag, costs, optionShowCosts);

		} catch (std::exception& e) {

			LOG_USER(logger::out) << "could not find costs" << std::endl;
		}

		// get volume rays

		std::shared_ptr<VolumeRays> rays = std::make_shared<VolumeRays>(crag);

		try {

			cragStore.retrieveVolumeRays(*rays);

		} catch (std::exception& e) {

			LOG_USER(logger::out) << "could not find volume rays" << std::endl;
			rays.reset();
		}

		// visualize

		auto cragView       = std::make_shared<CragView>();
		auto meshController = std::make_shared<MeshViewController>(crag, volumes, supervoxels);
		auto costsView      = std::make_shared<CostsView>(crag, costs, optionShowCosts);
		auto featuresView   = std::make_shared<SoltionView>(crag, nodeFeatures, edgeFeatures);
		auto rotateView     = std::make_shared<RotateView>();
		auto zoomView       = std::make_shared<ZoomView>(true);
		auto window         = std::make_shared<sg_gui::Window>("CRAG viewer");

		if (showSolutionCandidates)
			meshController->setSolution(overlaySolution);

		window->add(zoomView);
		zoomView->add(rotateView);
		rotateView->add(cragView);
		rotateView->add(meshController);
		rotateView->add(costsView);

		if (optionShowFeatures)
			rotateView->add(featuresView);

		if (optionShowSolution) {

			auto solutionView = std::make_shared<SolutionView>(crag, *viewSolution, optionShowSolution.as<std::string>());
			rotateView->add(solutionView);
		}

		cragView->setRawVolume(intensities);
		cragView->setLabelVolumes(overlays);

		if (rays)
			cragView->setVolumeRays(rays);

		window->processEvents();

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

