#include <map>
#include <tests.h>
#include <crag/Crag.h>
#include <io/Hdf5CragStore.h>

void hdf5_store() {

	Crag crag;
	CragVolumes volumes(crag);

	for (int i = 0; i < 100; i++)
		crag.addNode();

	for (int i = 0; i < 100; i++)
		for (int j = 0; j < 100; j++)
			if (rand() > RAND_MAX/2)
				crag.addAdjacencyEdge(
						crag.nodeFromId(i),
						crag.nodeFromId(j));

	for (int i = 0; i < 100; i += 5) {

		std::shared_ptr<CragVolume> volume = std::make_shared<CragVolume>(5, 5, 5);
		volume->setOffset(rand(), rand(), rand());
		volume->setResolution(rand(), rand(), rand());

		for (unsigned char& v : volume->data())
			v = rand()%256;

		volumes.setVolume(crag.nodeFromId(i), volume);

		for (int j = i; j < i + 4; j++)
			crag.addSubsetArc(
					crag.nodeFromId(j),
					crag.nodeFromId(j+1));
	}

	Hdf5CragStore store("test.hdf");

	store.saveCrag(crag);
	store.saveVolumes(volumes);

	Crag crag_;
	CragVolumes volumes_(crag_);

	store.retrieveCrag(crag_);
	store.retrieveVolumes(volumes_);

	for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n) {

		BOOST_CHECK_EQUAL(
				crag.isLeafNode(n),
				crag_.isLeafNode(n));

		BOOST_CHECK_EQUAL(
				crag.isRootNode(n),
				crag_.isRootNode(n));

		if (crag.isLeafNode(n)) {

			std::shared_ptr<CragVolume> a = volumes[n];
			std::shared_ptr<CragVolume> b = volumes_[n];

			BOOST_CHECK_EQUAL(
					a->getResolution(),
					b->getResolution());
			BOOST_CHECK_EQUAL(
					a->getOffset(),
					b->getOffset());

			BOOST_CHECK(a->data() == b->data());
		}
	}

	for (Crag::SubsetArcIt e(crag_); e != lemon::INVALID; ++e)
		BOOST_CHECK(
				crag_.id(crag_.getSubsetGraph().source(e)) == crag_.id(crag_.getSubsetGraph().target(e)) - 1);
}
