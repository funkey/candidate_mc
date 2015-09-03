#ifndef CANDIDATE_MC_LEARNING_TOPOLOGICAL_LOSS_H__
#define CANDIDATE_MC_LEARNING_TOPOLOGICAL_LOSS_H__

#include "Loss.h"
#include "BestEffort.h"
/**
 * Implementation of the topological loss proposed in:
 *
 *   Jan Funke, Fred A. Hamprecht, Chong Zhang
 *   "Learning to Segment: Training Hierarchical Segmentation under a 
 *   Topological Loss"
 *   MICCAI 2015
 *
 * The loss is only defined on nodes, on edges it is zero.
 */
class TopologicalLoss : public Loss {

public:

	/**
	 * Create a new topological loss from a given best-effort solution.
	 *
	 * @param crag
	 *              The CRAG the best-effort solution was computed for.
	 * @param bestEffort
	 *              The best-effort solution.
	 */
	TopologicalLoss(const Crag& crag, const BestEffort& bestEffort);

private:

	struct NodeCosts {

		double split;
		double merge;
		double fp;
		double fn;

		operator double() const {

			return split + merge + fp + fn;
		}
	};

	NodeCosts traverseAboveBestEffort(const Crag& crag, Crag::CragNode node, const BestEffort& bestEffort);

	void traverseBelowBestEffort(const Crag& crag, Crag::CragNode node, NodeCosts costs);

	double _weightSplit;
	double _weightMerge;
	double _weightFp;
	double _weightFn;
};

#endif // CANDIDATE_MC_LEARNING_TOPOLOGICAL_LOSS_H__

