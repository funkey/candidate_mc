#include "SmallFirst.h"

util::ProgramOption optionSmallRegionThreshold1(
		util::_long_name        = "smallRegionThreshold1",
		util::_description_text = "Maximal size of a region to be considered small. Small regions are merged in a first pass before others are considered.",
		util::_default_value    = 50);

util::ProgramOption optionSmallRegionThreshold2(
		util::_long_name        = "smallRegionThreshold2",
		util::_description_text = "Maximal size of a region to be considered small, when their average intensity is also above 'intensityThreshold'. Small regions are merged in a first pass before others are considered.",
		util::_default_value    = 100);

util::ProgramOption optionIntensityThreshold(
		util::_long_name        = "intensityThreshold",
		util::_description_text = "Intensity threshold for small regions according to 'smallRegionThreshold2'.",
		util::_default_value    = 0.8);

