#include <map>
#include <tests.h>
#include <crag/Crag.h>

namespace create_crag_case {

} using namespace create_crag_case;

void create_crag() {

	// useful for later:
	//
	//boost::filesystem::path dataDir = dir_of(__FILE__);
	//boost::filesystem::path graphfile = dataDir/"star.dat";

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

	// check levels of nodes
	for (int i = 0; i < 100; i++) {

		Crag::Node n = crag.nodeFromId(i);
		BOOST_CHECK_EQUAL(crag.getLevel(n), i%10);

		if (i%10 == 0)
			BOOST_CHECK(crag.isLeafNode(n));
		else
			BOOST_CHECK(!crag.isLeafNode(n));

		if (i%10 == 9)
			BOOST_CHECK(crag.isRootNode(n));
		else
			BOOST_CHECK(!crag.isRootNode(n));
	}
}
