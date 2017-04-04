define_module(pycmc LIBRARY LINKS crag inference imageprocessing io boost-python)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_custom_target(rename_pycmc_lib ALL COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/python/libpycmc.dylib ${CMAKE_BINARY_DIR}/python/pycmc.so)
else()
  add_custom_target(rename_pycmc_lib ALL COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/python/libpycmc.so ${CMAKE_BINARY_DIR}/python/pycmc.so)
endif()

add_dependencies(rename_pycmc_lib pycmc)
