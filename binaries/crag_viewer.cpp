/**
 * This programs visualizes a CRAG stored in an hdf5 file.
 */

#include <io/Hdf5CragStore.h>
#include <io/Hdf5VolumeStore.h>
#include <util/ProgramOptions.h>
#include <imageprocessing/ExplicitVolume.h>
#include <gui/CragView.h>
#include <gui/MeshViewController.h>
#include <sg_gui/RotateView.h>
#include <sg_gui/ZoomView.h>
#include <sg_gui/Window.h>

using namespace sg_gui;

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The project file to read the CRAG from.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionShowLabels(
		util::_long_name        = "showLabels",
		util::_description_text = "Instead of showing the CRAG leaf nodes, show the labels (i.e., a segmentation) stored in the project file.");

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		// create hdf5 stores

		Hdf5CragStore   cragStore(optionProjectFile.as<std::string>());
		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());

		// get crag

		Crag crag;

		try {

			cragStore.retrieveCrag(crag);

		} catch (std::exception& e) {

			LOG_USER(logger::out) << "could not find a CRAG" << std::endl;
		}

		// get intensities and supervoxel labels

		auto intensities = std::make_shared<ExplicitVolume<float>>();
		volumeStore.retrieveIntensities(*intensities);
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

		if (optionShowLabels) {

			ExplicitVolume<int> labels;
			volumeStore.retrieveLabels(labels);
			supervoxels->data() = labels.data();

		} else {

			for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

				if (!crag.isLeafNode(n))
					continue;

				const ExplicitVolume<unsigned char>& volume = crag.getVolume(n);
				util::point<int, 3> offset = volume.getOffset()/volume.getResolution();

				for (int z = 0; z < volume.getDiscreteBoundingBox().depth();  z++)
				for (int y = 0; y < volume.getDiscreteBoundingBox().height(); y++)
				for (int x = 0; x < volume.getDiscreteBoundingBox().width();  x++) {

					if (volume.data()(x, y, z))
						(*supervoxels)(
								offset.x() + x,
								offset.y() + y,
								offset.z() + z)
										= crag.id(n);
				}
			}
		}

		// visualize

		auto cragView       = std::make_shared<CragView>();
		auto meshController = std::make_shared<MeshViewController>(crag, supervoxels);
		auto rotateView     = std::make_shared<RotateView>();
		auto zoomView       = std::make_shared<ZoomView>(true);
		auto window         = std::make_shared<sg_gui::Window>("CRAG viewer");

		window->add(zoomView);
		zoomView->add(rotateView);
		rotateView->add(cragView);
		rotateView->add(meshController);

		cragView->setRawVolume(intensities);
		cragView->setLabelsVolume(supervoxels);

		window->processEvents();

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

