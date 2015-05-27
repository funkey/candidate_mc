#ifndef QUADRATIC_PROGRAM_SOLVER_FACTORY_H__
#define QUADRATIC_PROGRAM_SOLVER_FACTORY_H__

#include "QuadraticSolverBackend.h"

class QuadraticSolverBackendFactory {

public:

	virtual QuadraticSolverBackend* createQuadraticSolverBackend() const = 0;
};

#endif // QUADRATIC_PROGRAM_SOLVER_FACTORY_H__

