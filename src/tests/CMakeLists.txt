# Set of tests
set(TESTS test_rng)

#~ # Add current folder as an include folder
#~ include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add a target for each test
foreach(TEST ${TESTS})
	add_executable(${TEST} ${TEST}.c)
	target_link_libraries(${TEST} ${PROJECT_NAME} ${CF4OCL2_LIBRARIES}
		${OPENCL_LIBRARIES} ${GLIB_LIBRARIES} ${GLIB_LDFLAGS})
endforeach(TEST)

# Add a target which builds all tests
add_custom_target(tests DEPENDS ${TESTS})

