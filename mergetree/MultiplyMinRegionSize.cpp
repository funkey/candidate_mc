#include "MultiplyMinRegionSize.h"

util::ProgramOption optionMultiplyMinRegionSizeExponent(
		util::_long_name        = "minRegionSizeExponent",
		util::_description_text = "The parameter α for the MultiplyMinRegionSize scoring function: the score is s*pow(minRegionSize, α), where s is the score of another function. Default is 1.",
		util::_default_value    = 1);
