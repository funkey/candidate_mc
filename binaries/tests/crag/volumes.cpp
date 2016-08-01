#include <tests.h>
#include <crag/Crag.h>
#include <crag/CragVolumes.h>

void volumes() {

	Crag crag;
	CragVolumes volumes(crag);

	// add a few nodes
	for (int i = 0; i < 8; i++)
		crag.addNode();

	/*     7
	 *   /  \
	 *  5    6
	 * / \ / | \
	 * 0 1 2 3 4
	 */

	// add subset relations
	crag.addSubsetArc(crag.nodeFromId(0), crag.nodeFromId(5));
	crag.addSubsetArc(crag.nodeFromId(1), crag.nodeFromId(5));
	crag.addSubsetArc(crag.nodeFromId(2), crag.nodeFromId(6));
	crag.addSubsetArc(crag.nodeFromId(3), crag.nodeFromId(6));
	crag.addSubsetArc(crag.nodeFromId(4), crag.nodeFromId(6));
	crag.addSubsetArc(crag.nodeFromId(5), crag.nodeFromId(7));
	crag.addSubsetArc(crag.nodeFromId(6), crag.nodeFromId(7));

	// leaf node volumes
	std::shared_ptr<CragVolume> v0 = std::make_shared<CragVolume>(10, 10, 10);
	std::shared_ptr<CragVolume> v1 = std::make_shared<CragVolume>(10, 10, 10);
	std::shared_ptr<CragVolume> v2 = std::make_shared<CragVolume>(10, 10, 10);
	std::shared_ptr<CragVolume> v3 = std::make_shared<CragVolume>(10, 10, 10);
	std::shared_ptr<CragVolume> v4 = std::make_shared<CragVolume>(10, 10, 10);
	v0->setOffset(0, 0, 0);
	v1->setOffset(1, 1, 1);
	v2->setOffset(2, 2, 2);
	v3->setOffset(3, 3, 3);
	v4->setOffset(4, 4, 4);

	volumes.setVolume(crag.nodeFromId(0), v0);
	volumes.setVolume(crag.nodeFromId(1), v1);
	volumes.setVolume(crag.nodeFromId(2), v2);
	volumes.setVolume(crag.nodeFromId(3), v3);
	volumes.setVolume(crag.nodeFromId(4), v4);

	BOOST_CHECK_EQUAL(volumes[crag.nodeFromId(5)]->getBoundingBox(), v0->getBoundingBox() + v1->getBoundingBox());
	BOOST_CHECK_EQUAL(volumes[crag.nodeFromId(6)]->getBoundingBox(), v2->getBoundingBox() + v4->getBoundingBox());
	BOOST_CHECK_EQUAL(volumes[crag.nodeFromId(7)]->getBoundingBox(), v0->getBoundingBox() + v4->getBoundingBox());
}
