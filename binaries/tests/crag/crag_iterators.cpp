#include <map>
#include <tests.h>
#include <crag/Crag.h>
#include <util/timing.h>

void crag_iterators() {

	Crag crag;

	for (int i = 0; i < 100; i++)
		crag.addNode();

	for (int i = 0; i < 100; i++)
		for (int j = 0; j < 100; j++)
			if (rand() > RAND_MAX/2)
				crag.addAdjacencyEdge(
						crag.nodeFromId(i),
						crag.nodeFromId(j));

	for (int i = 0; i < 100; i += 5) {

		for (int j = i; j < i + 4; j++)
			crag.addSubsetArc(
					crag.nodeFromId(j),
					crag.nodeFromId(j+1));
	}

	int numNodes = 0;

	//{
		//UTIL_TIME_SCOPE("warm up");

		//for (int j = 0; j < 100; j++)
			//for (Crag::CragNode n : crag.nodes())
				//numNodes++;
	//}

	//numNodes = 0;

	{
		//UTIL_TIME_SCOPE("crag node old-school iterator");

		//for (int j = 0; j < 1e8; j++)
			for (Crag::CragNodeIterator i = crag.nodes().begin(); i != crag.nodes().end(); i++)
				numNodes++;
	}

	{
		//UTIL_TIME_SCOPE("crag node iterator");

		//for (int j = 0; j < 1e8; j++)
			for (Crag::CragNode n : crag.nodes())
				numNodes++;
	}

	{
		//UTIL_TIME_SCOPE("lemon node iterator");

		//for (int j = 0; j < 1e8; j++)
			for (Crag::NodeIt n(crag); n != lemon::INVALID; ++n)
				numNodes -= 2;
	}

	BOOST_CHECK_EQUAL(numNodes, 0);

	int numEdges = 0;

	for (Crag::CragEdge e : crag.edges())
		numEdges++;

	for (Crag::EdgeIt e(crag); e != lemon::INVALID; ++e)
		numEdges--;

	BOOST_CHECK_EQUAL(numEdges, 0);

	int numArcs = 0;

	for (Crag::CragArc a : crag.arcs())
		numArcs++;

	for (Crag::SubsetArcIt a(crag); a != lemon::INVALID; ++a)
		numArcs--;

	BOOST_CHECK_EQUAL(numArcs, 0);

	for (Crag::CragNode n : crag.nodes()) {

		int numAdjEdges = 0;
		for (Crag::CragEdge e : n.adjedges())
			numAdjEdges++;
		for (Crag::IncEdgeIt e(crag, n); e != lemon::INVALID; ++e)
			numAdjEdges--;
		BOOST_CHECK_EQUAL(numAdjEdges, 0);

		int numInArcs = 0;
		for (Crag::CragArc a : n.inarcs())
			numInArcs++;
		for (Crag::SubsetInArcIt a(crag, crag.toSubset(n)); a != lemon::INVALID; ++a)
			numInArcs--;
		BOOST_CHECK_EQUAL(numInArcs, 0);

		int numOutArcs = 0;
		for (Crag::CragArc a : n.outarcs())
			numOutArcs++;
		for (Crag::SubsetOutArcIt a(crag, crag.toSubset(n)); a != lemon::INVALID; ++a)
			numOutArcs--;
		BOOST_CHECK_EQUAL(numOutArcs, 0);

		Crag::CragEdgeIterator cei = crag.edges().begin();
		Crag::EdgeIt            ei(crag);

		while (ei != lemon::INVALID) {

			BOOST_CHECK((Crag::RagType::Node)((*cei).u()) == crag.getAdjacencyGraph().u(ei));
			BOOST_CHECK((Crag::RagType::Node)((*cei).v()) == crag.getAdjacencyGraph().v(ei));
			++cei;
			++ei;
		}

		Crag::CragArcIterator cai = crag.arcs().begin();
		Crag::SubsetArcIt      ai(crag);

		while (ai != lemon::INVALID) {

			BOOST_CHECK((Crag::SubsetType::Node)((*cai).source()) == crag.getSubsetGraph().source(ai));
			BOOST_CHECK((Crag::SubsetType::Node)((*cai).target()) == crag.getSubsetGraph().target(ai));
			++cai;
			++ai;
		}
	}
}
