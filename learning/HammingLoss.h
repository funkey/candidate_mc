#ifndef CANDIDATE_MC_LEARNING_HAMMING_LOSS_H__
#define CANDIDATE_MC_LEARNING_HAMMING_LOSS_H__

#include "Loss.h"
#include "BestEffort.h"

class HammingLoss : public Loss {

public:

	HammingLoss(const Crag& crag, const BestEffort& bestEffort);
};

#endif // CANDIDATE_MC_LEARNING_HAMMING_LOSS_H__

