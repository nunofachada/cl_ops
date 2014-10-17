# List of tests
set(TESTS clo_scan_test clo_rng_test clo_sort_test)

# Common libs for tests
set(TEST_LIBS cl_ops ${OPENCL_LIBRARIES} ${GLIB_LIBRARIES})

# Add a target for each utility
foreach(TEST ${TESTS})
	add_executable(${TEST} ${TEST}.c clo_test.c)
	target_link_libraries(${TEST} ${TEST_LIBS})
endforeach()

# Tests requiring OCL code
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/clo_rng_test.cl RNG_TEST_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" RNG_TEST_SRC ${RNG_TEST_SRC_RAW})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_rng_test.in.h
	${CMAKE_BINARY_DIR}/cl_ops/clo_rng_test.h @ONLY)

# How to install tests
install(TARGETS ${TESTS} RUNTIME DESTINATION bin)

# Add a target which builds all utilities
add_custom_target(tests DEPENDS ${TESTS})

