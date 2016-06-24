#include <tests.h>
#include <inference/ClosedSetSolver.h>

void closed_set_solver() {

	/**
	 *  Subsets:
	 *              n7
	 *            /    \
	 *           /      \
	 *          /        \
	 *         n5        n6
	 *        / \       /  \
	 *      n1   n2    n3   n4
	 *
	 *  Adjacencies:
	 *              n7
	 *
	 *
	 *              d
	 *         n5--------n6
	 *           \     /
	 *
	 *          e  \ /  f
	 *
	 *             / \
	 *      n1---n2----n3---n4
	 *         a    b     c
	 */

	Crag crag;
	Crag::CragNode n1 = crag.addNode();
	Crag::CragNode n2 = crag.addNode();
	Crag::CragNode n3 = crag.addNode();
	Crag::CragNode n4 = crag.addNode();
	Crag::CragNode n5 = crag.addNode();
	Crag::CragNode n6 = crag.addNode();
	Crag::CragNode n7 = crag.addNode();

	crag.addSubsetArc(n1, n5);
	crag.addSubsetArc(n2, n5);
	crag.addSubsetArc(n3, n6);
	crag.addSubsetArc(n4, n6);
	crag.addSubsetArc(n5, n7);
	crag.addSubsetArc(n6, n7);

	Crag::CragEdge a = crag.addAdjacencyEdge(n1, n2);
	Crag::CragEdge b = crag.addAdjacencyEdge(n2, n3);
	Crag::CragEdge c = crag.addAdjacencyEdge(n3, n4);
	Crag::CragEdge d = crag.addAdjacencyEdge(n5, n6);
	Crag::CragEdge e = crag.addAdjacencyEdge(n5, n3);
	Crag::CragEdge f = crag.addAdjacencyEdge(n2, n6);

	ClosedSetSolver solver(crag);
	CragSolution x(crag);
	ClosedSetSolver::Status status;

	{
		Costs costs(crag);
		costs.node[n7] = -1;
		solver.setCosts(costs);
		status = solver.solve(x);

		BOOST_CHECK_EQUAL(status, ClosedSetSolver::SolutionFound);
		// everything should be turned on
		for (Crag::CragNode n : crag.nodes())
			BOOST_CHECK(x.selected(n));
		for (Crag::CragEdge e : crag.edges())
			BOOST_CHECK(x.selected(e));
	}

	{
		Costs costs(crag);
		costs.node[n7] = 1;
		costs.edge[d] = -1;
		solver.setCosts(costs);
		status = solver.solve(x);

		BOOST_CHECK_EQUAL(status, ClosedSetSolver::SolutionFound);
		// everything except n7 should be turned on
		for (Crag::CragNode n : crag.nodes())
			BOOST_CHECK(n != n7 ? x.selected(n) : !x.selected(n));
		for (Crag::CragEdge e : crag.edges())
			BOOST_CHECK(x.selected(e));
	}
}
