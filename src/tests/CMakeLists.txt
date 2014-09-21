# List of tests
set(TESTS clo_scan_test)

# Common libs for tests
set(TEST_LIBS cl_ops ${OPENCL_LIBRARIES} ${GLIB_LIBRARIES})

# Add a target for each utility
foreach(TEST ${TESTS})
	add_executable(${TEST} ${TEST}.c)
	target_link_libraries(${TEST} ${TEST_LIBS})
endforeach()

# How to install tests
install(TARGETS ${TESTS} RUNTIME DESTINATION bin)

# Add a target which builds all utilities
add_custom_target(tests DEPENDS ${TESTS})

