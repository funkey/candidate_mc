#include <tests.h>
#include <crag/Crag.h>
#include <learning/BestEffort.h>
#include <learning/HammingLoss.h>

void hamming_loss() {

	Crag crag;

	Crag::CragNode a = crag.addNode();
	Crag::CragNode b = crag.addNode();
	Crag::CragNode c = crag.addNode();
	Crag::CragNode d = crag.addNode();
	Crag::CragNode e = crag.addNode();
	Crag::CragNode f = crag.addNode();
	Crag::CragNode g = crag.addNode();

	crag.addSubsetArc(a, e);
	crag.addSubsetArc(b, e);
	crag.addSubsetArc(c, f);
	crag.addSubsetArc(d, f);
	crag.addSubsetArc(e, g);
	crag.addSubsetArc(f, g);

	Crag::CragEdge ab = crag.addAdjacencyEdge(a, b);
	Crag::CragEdge bc = crag.addAdjacencyEdge(b, c);
	Crag::CragEdge cd = crag.addAdjacencyEdge(c, d);
	Crag::CragEdge ef = crag.addAdjacencyEdge(e, f);

	// empty best-effort
	{
		BestEffort bestEffort(crag);

		HammingLoss hamming(crag, bestEffort, true /* balance */);

		BOOST_CHECK_EQUAL(hamming.node[a], 1);
		BOOST_CHECK_EQUAL(hamming.node[b], 1);
		BOOST_CHECK_EQUAL(hamming.node[c], 1);
		BOOST_CHECK_EQUAL(hamming.node[d], 1);
		BOOST_CHECK_EQUAL(hamming.node[e], 3);
		BOOST_CHECK_EQUAL(hamming.node[f], 3);
		BOOST_CHECK_EQUAL(hamming.node[g], 7);

		BOOST_CHECK_EQUAL(hamming.edge[ab], 1);
		BOOST_CHECK_EQUAL(hamming.edge[bc], 1);
		BOOST_CHECK_EQUAL(hamming.edge[cd], 1);
		BOOST_CHECK_EQUAL(hamming.edge[ef], 1);

		BOOST_CHECK_EQUAL(hamming.constant, 0);
	}

	// best effort on leaf nodes and edges
	{
		BestEffort bestEffort(crag);

		bestEffort.node[a] = true;
		bestEffort.node[b] = true;
		bestEffort.node[c] = true;
		bestEffort.node[d] = true;

		bestEffort.edge[ab] = true;
		bestEffort.edge[bc] = true;
		bestEffort.edge[cd] = true;

		HammingLoss hamming(crag, bestEffort, true /* balance */);

		BOOST_CHECK_EQUAL(hamming.node[a], -1);
		BOOST_CHECK_EQUAL(hamming.node[b], -1);
		BOOST_CHECK_EQUAL(hamming.node[c], -1);
		BOOST_CHECK_EQUAL(hamming.node[d], -1);
		BOOST_CHECK_EQUAL(hamming.node[e], -3);
		BOOST_CHECK_EQUAL(hamming.node[f], -3);
		BOOST_CHECK_EQUAL(hamming.node[g], -7);

		BOOST_CHECK_EQUAL(hamming.edge[ab], -1);
		BOOST_CHECK_EQUAL(hamming.edge[bc], -1);
		BOOST_CHECK_EQUAL(hamming.edge[cd], -1);
		BOOST_CHECK_EQUAL(hamming.edge[ef], -1);

		BOOST_CHECK_EQUAL(hamming.constant, 7);
	}

	// best effort on root node
	{
		BestEffort bestEffort(crag);

		bestEffort.node[g] = true;

		HammingLoss hamming(crag, bestEffort, true /* balance */);

		BOOST_CHECK_EQUAL(hamming.node[a], -1);
		BOOST_CHECK_EQUAL(hamming.node[b], -1);
		BOOST_CHECK_EQUAL(hamming.node[c], -1);
		BOOST_CHECK_EQUAL(hamming.node[d], -1);
		BOOST_CHECK_EQUAL(hamming.node[e], -3);
		BOOST_CHECK_EQUAL(hamming.node[f], -3);
		BOOST_CHECK_EQUAL(hamming.node[g], -7);

		BOOST_CHECK_EQUAL(hamming.edge[ab], -1);
		BOOST_CHECK_EQUAL(hamming.edge[bc], -1);
		BOOST_CHECK_EQUAL(hamming.edge[cd], -1);
		BOOST_CHECK_EQUAL(hamming.edge[ef], -1);

		BOOST_CHECK_EQUAL(hamming.constant, 7);
	}

	// best effort on e and f
	{
		BestEffort bestEffort(crag);

		bestEffort.node[e] = true;
		bestEffort.node[f] = true;

		HammingLoss hamming(crag, bestEffort, true /* balance */);

		BOOST_CHECK_EQUAL(hamming.node[a], -1);
		BOOST_CHECK_EQUAL(hamming.node[b], -1);
		BOOST_CHECK_EQUAL(hamming.node[c], -1);
		BOOST_CHECK_EQUAL(hamming.node[d], -1);
		BOOST_CHECK_EQUAL(hamming.node[e], -3);
		BOOST_CHECK_EQUAL(hamming.node[f], -3);
		BOOST_CHECK_EQUAL(hamming.node[g], -5);

		BOOST_CHECK_EQUAL(hamming.edge[ab], -1);
		BOOST_CHECK_EQUAL(hamming.edge[bc], 1);
		BOOST_CHECK_EQUAL(hamming.edge[cd], -1);
		BOOST_CHECK_EQUAL(hamming.edge[ef], 1);

		BOOST_CHECK_EQUAL(hamming.constant, 6);
	}

	// best effort on e and c-d
	{
		BestEffort bestEffort(crag);

		bestEffort.node[e] = true;
		bestEffort.node[c] = true;
		bestEffort.node[d] = true;

		bestEffort.edge[cd] = true;

		HammingLoss hamming(crag, bestEffort, true /* balance */);

		BOOST_CHECK_EQUAL(hamming.node[a], -1);
		BOOST_CHECK_EQUAL(hamming.node[b], -1);
		BOOST_CHECK_EQUAL(hamming.node[c], -1);
		BOOST_CHECK_EQUAL(hamming.node[d], -1);
		BOOST_CHECK_EQUAL(hamming.node[e], -3);
		BOOST_CHECK_EQUAL(hamming.node[f], -3);
		BOOST_CHECK_EQUAL(hamming.node[g], -5);

		BOOST_CHECK_EQUAL(hamming.edge[ab], -1);
		BOOST_CHECK_EQUAL(hamming.edge[bc], 1);
		BOOST_CHECK_EQUAL(hamming.edge[cd], -1);
		BOOST_CHECK_EQUAL(hamming.edge[ef], 1);

		BOOST_CHECK_EQUAL(hamming.constant, 6);
	}
}
