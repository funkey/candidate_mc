#ifndef MULTI2CUT_MERGETREE_RANDOM_PERTURBATION_H__
#define MULTI2CUT_MERGETREE_RANDOM_PERTURBATION_H__

#include <boost/math/distributions/normal.hpp>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include "ScoringFunction.h"

extern util::ProgramOption optionRandomPerturbationStdDev;
extern util::ProgramOption optionRandomPerturbationSeed;
extern logger::LogChannel randomperturbationlog;

/**
 * A scoring function that randomly perturbes the scores of another scoring 
 * function. For that, perturbations are drawn from a normal distrubution with 
 * standard deviation of optionRandomPerturbationStdDev.
 */
template <typename ScoringFunctionType>
class RandomPerturbation : public ScoringFunction {

public:

	RandomPerturbation(ScoringFunctionType& scoringFunction) :
		_scoringFunction(scoringFunction),
		_normalDistribution(0, 1),
		_stdDev(optionRandomPerturbationStdDev) {

			LOG_USER(randomperturbationlog)
					<< "randomly perturb edge scores with stddev "
					<< optionRandomPerturbationStdDev.as<double>()
					<< std::endl;

			srand(optionRandomPerturbationSeed.as<int>());
		}

	float operator()(const RagType::Edge& edge, std::vector<GridGraphType::Edge>& gridEdges) {

		float score = _scoringFunction(edge, gridEdges);

		double uniform = (double)rand()/RAND_MAX;
		double pertubation = boost::math::quantile(_normalDistribution, uniform);

		pertubation = pertubation*_stdDev*1.0/gridEdges.size();

		return score + pertubation;
	}

	void onMerge(const RagType::Edge& edge, const RagType::Node newRegion) {

		_scoringFunction.onMerge(edge, newRegion);
	}

private:

	ScoringFunctionType&               _scoringFunction;
	boost::math::normal_distribution<> _normalDistribution;

	// the baseline stddev
	double _stdDev;
};

#endif // MULTI2CUT_MERGETREE_RANDOM_PERTURBATION_H__

