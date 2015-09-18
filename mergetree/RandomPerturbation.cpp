#include "RandomPerturbation.h"

util::ProgramOption optionRandomPerturbationStdDev(
		util::_long_name        = "randomPerturbationStdDev",
		util::_description_text = "The standard deviation of the normal distribution to be used to randomly perturb the edge scores.",
		util::_default_value    = 50);

util::ProgramOption optionRandomPerturbationSeed(
		util::_long_name        = "randomPerturbationSeed",
		util::_description_text = "The seed for the random number generator.",
		util::_default_value    = 7 /* why not? */);

logger::LogChannel randomperturbationlog("randomperturbationlog", "[RandomPerturbation] ");
