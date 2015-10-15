#include "MultiplySizeDifference.h"

util::ProgramOption optionMultiplySizeDifferenceExponent(
		util::_long_name        = "sizeDifferenceExponent",
		util::_description_text = "The parameter α for the MultiplySizeDifference scoring function: the score is s*pow(sizeDifference, α), where s is the score of another function. Default is 1.",
		util::_default_value    = 1);

