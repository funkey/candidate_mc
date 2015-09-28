#include <config.h>
#include <tests.h>
#include <solver/SolverFactory.h>

namespace backends_test {

int numVars = 10;

LinearObjective   objective(numVars);
LinearConstraints constraints;

void testSolver(LinearSolverBackend* solver) {

	Solution x(numVars);
	std::string _;

	solver->initialize(numVars, Binary);

	objective.setSense(Minimize);
	solver->setObjective(objective);
	solver->setConstraints(constraints);
	solver->solve(x, _);

	BOOST_CHECK_EQUAL(x.getValue(), -3999);
	BOOST_CHECK_EQUAL(x[numVars - 1], 1);
	for (int i = 0; i < numVars - 1; i++)
		BOOST_CHECK_EQUAL(x[i], 0);

	objective.setSense(Maximize);
	solver->setObjective(objective);
	solver->setConstraints(constraints);
	solver->solve(x, _);

	BOOST_CHECK_EQUAL(x.getValue(), 5001);
	BOOST_CHECK_EQUAL(x[0], 1);
	for (int i = 1; i < numVars; i++)
		BOOST_CHECK_EQUAL(x[i], 0);
}

} using namespace backends_test;

void backends() {

	for (int i = 0; i < numVars; i++)
		objective.setCoefficient(i, 1000*(5 - i%10));
	objective.setConstant(1);

	LinearConstraint onlyOneConstraint;
	for (int i = 0; i < numVars; i++)
		onlyOneConstraint.setCoefficient(i, 1.0);
	onlyOneConstraint.setRelation(LessEqual);
	onlyOneConstraint.setValue(1.0);

	constraints.add(onlyOneConstraint);

	SolverFactory factory;

#ifdef HAVE_GUROBI
	{
		std::cout << "testing gurobi solver" << std::endl;
		LinearSolverBackend* solver = factory.createLinearSolverBackend(Gurobi);
		testSolver(solver);
		delete solver;
	}
#endif

#ifdef HAVE_CPLEX
	{
		std::cout << "testing cplex solver" << std::endl;
		LinearSolverBackend* solver = factory.createLinearSolverBackend(Cplex);
		testSolver(solver);
		delete solver;
	}
#endif

#ifdef HAVE_SCIP
	{
		std::cout << "testing scip solver" << std::endl;
		LinearSolverBackend* solver = factory.createLinearSolverBackend(Scip);
		testSolver(solver);
		delete solver;
	}
#endif
}
