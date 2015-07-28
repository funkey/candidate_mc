#include "DefaultFactory.h"

#include <config.h>
#include <util/ProgramOptions.h>

#ifdef HAVE_GUROBI
#include "GurobiBackend.h"
#endif

#ifdef HAVE_CPLEX
#include "CplexBackend.h"
#endif

util::ProgramOption optionUseGurobi(
		util::_long_name        = "useGurobi",
		util::_description_text = "Use the gurobi solver for ILPs and QPs. If not set, the first "
		                          "available solver will be used."
);

util::ProgramOption optionUseCplex(
		util::_long_name        = "useCplex",
		util::_description_text = "Use the CPLEX solver for ILPs and QPs. If not set, the first "
		                          "available solver will be used."
);

LinearSolverBackend*
DefaultFactory::createLinearSolverBackend() const {

	enum Preference { Any, Cplex, Gurobi };

	Preference preference = Any;

	if (optionUseGurobi && optionUseCplex)
		UTIL_THROW_EXCEPTION(
				LinearSolverBackendException,
				"only one solver can be chosen");

	if (optionUseGurobi)
		preference = Gurobi;
	if (optionUseCplex)
		preference = Cplex;

// by default, create a gurobi backend
#ifdef HAVE_GUROBI

	if (preference == Any || preference == Gurobi) {

		try {

			return new GurobiBackend();

		} catch (GRBException& e) {

			UTIL_THROW_EXCEPTION(
					LinearSolverBackendException,
					"gurobi error: " << e.getMessage());
		}
	}

#endif

// if this is not available, create a CPLEX backend
#ifdef HAVE_CPLEX

	if (preference == Any || preference == Cplex)
		return new CplexBackend();

#endif

// if this is not available as well, throw an exception

	BOOST_THROW_EXCEPTION(NoSolverException() << error_message("No linear solver available."));
}

QuadraticSolverBackend*
DefaultFactory::createQuadraticSolverBackend() const {

// by default, create a gurobi backend
#ifdef HAVE_GUROBI

	return new GurobiBackend();

#endif

// if this is not available, create a CPLEX backend
#ifdef HAVE_CPLEX

	return new CplexBackend();

#endif

// if this is not available as well, throw an exception

	BOOST_THROW_EXCEPTION(NoSolverException() << error_message("No linear solver available."));
}
