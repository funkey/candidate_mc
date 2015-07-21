#ifndef CANDIDATE_MC_LEARNING_ORACLE_H__
#define CANDIDATE_MC_LEARNING_ORACLE_H__

#include <crag/Crag.h>
#include <inference/MultiCut.h>
#include <features/NodeFeatures.h>
#include <features/EdgeFeatures.h>
#include "Loss.h"
#include "BestEffort.h"

/**
 * Provides solution for loss-augmented inference problem, given a set of 
 * weights. To be used in a learning optimizer.
 */
class Oracle {

public:

	Oracle(
			const Crag&                  crag,
			const CragVolumes&           volumes,
			const NodeFeatures&          nodeFeatures,
			const EdgeFeatures&          edgeFeatures,
			const Loss&                  loss,
			const BestEffort&            bestEffort,
			MultiCut::Parameters         parameters = MultiCut::Parameters()) :
		_crag(crag),
		_volumes(volumes),
		_nodeFeatures(nodeFeatures),
		_edgeFeatures(edgeFeatures),
		_loss(loss),
		_bestEffort(bestEffort),
		_costs(_crag),
		_mostViolatedMulticut(crag, parameters),
		_currentBestMulticut(crag, parameters),
		_iteration(0) {}

	void operator()(
			const std::vector<double>& weights,
			double&                    value,
			std::vector<double>&       gradient);

private:

	void updateCosts(const std::vector<double>& weights);

	void accumulateGradient(std::vector<double>& gradient);

	inline double nodeCost(const std::vector<double>& weights, const std::vector<double>& nodeFeatures) const {

		return dot(weights.begin(), weights.begin() + _nodeFeatures.dims(), nodeFeatures.begin());
	}

	inline double edgeCost(const std::vector<double>& weights, const std::vector<double>& edgeFeatures) const {

		return dot(weights.begin() + _nodeFeatures.dims(), weights.end(), edgeFeatures.begin());
	}

	template <typename IT>
	inline double dot(IT ba, IT ea, IT bb) const {

		double sum = 0;
		while (ba != ea) {

			sum += (*ba)*(*bb);
			ba++;
			bb++;
		}

		return sum;
	}

	const Crag&         _crag;
	const CragVolumes&  _volumes;
	const NodeFeatures& _nodeFeatures;
	const EdgeFeatures& _edgeFeatures;
	const Loss&         _loss;
	const BestEffort&   _bestEffort;

	Costs _costs;

	// constant to be added to the optimal value of the multi-cut solution
	double _constant;

	// best-effort part of _constant
	double _B_c;

	MultiCut _mostViolatedMulticut;
	MultiCut _currentBestMulticut;

	int _iteration;
};

#endif // CANDIDATE_MC_LEARNING_ORACLE_H__

