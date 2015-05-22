define_module(create_project           BINARY SOURCES create_project.cpp          LINKS crag imageprocessing io)
define_module(extract_features         BINARY SOURCES extract_features.cpp        LINKS crag features io util)
define_module(create_learning_problem  BINARY SOURCES create_learning_problem.cpp LINKS crag io util)
define_module(solve                    BINARY SOURCES solve.cpp                   LINKS crag inference io util)
