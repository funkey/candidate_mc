#include <map>
#include <tests.h>
#include <crag/Crag.h>

namespace modify_crag_case {

} using namespace modify_crag_case;

void modify_crag() {

	Crag crag;

	// add 100 nodes
	for (int i = 0; i < 100; i++)
		crag.addNode();

	// create chains of 10 nodes
	for (int i = 0; i < 100; i += 10) {

		for (int j = i+1; j < i + 10; j++) {

			Crag::Node u = crag.nodeFromId(j-1);
			Crag::Node v = crag.nodeFromId(j);
			crag.addSubsetArc(u, v);
		}
	}

	// remove some nodes

	for (int i = 5; i < 100; i += 10)
		crag.erase(crag.nodeFromId(i));

	// check levels of nodes
	for (int i = 0; i < 100; i++) {

		if (i%10 == 5)
			continue;

		Crag::Node n = crag.nodeFromId(i);

		if (i%10 < 5)
			BOOST_CHECK_EQUAL(crag.getLevel(n), i%10);
		else
			BOOST_CHECK_EQUAL(crag.getLevel(n), i%10 - 6);

		if (i%10 == 0 || i%10 == 6)
			BOOST_CHECK(crag.isLeafNode(n));
		else
			BOOST_CHECK(!crag.isLeafNode(n));

		if (i%10 == 4 || i%10 == 9)
			BOOST_CHECK(crag.isRootNode(n));
		else
			BOOST_CHECK(!crag.isRootNode(n));
	}
}

