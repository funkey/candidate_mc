define_module(create_project           BINARY SOURCES create_project.cpp          LINKS trag imageprocessing io)
define_module(extract_features         BINARY SOURCES extract_features.cpp        LINKS trag features io util)
define_module(create_learning_problem  BINARY SOURCES create_learning_problem.cpp LINKS trag io util)
define_module(solve                    BINARY SOURCES solve.cpp                   LINKS trag io util)
