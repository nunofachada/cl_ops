# Set of tests
set(TESTS test_rng)

# Add a target for each test
foreach(TEST ${TESTS})
	add_executable(${TEST} ${TEST}.c)
	target_link_libraries(${TEST} ${CF4OCL2_LIBRARIES}
		${GLIB_LIBRARIES} ${GLIB_LDFLAGS})
endforeach(TEST)

# Add a target which builds all tests
add_custom_target(tests DEPENDS ${TESTS})
