define_module(pycmc LIBRARY LINKS crag inference imageprocessing io boost-python)
add_custom_target(rename_pycmc_lib ALL COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/python/libpycmc.so ${CMAKE_BINARY_DIR}/python/pycmc.so)
add_dependencies(rename_pycmc_lib pycmc)
