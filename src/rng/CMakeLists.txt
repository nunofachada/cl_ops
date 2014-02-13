# Add RNG source to aggregated library sources list
set(CLO_LIB_SRCS_CURRENT clo_rng.c PARENT_SCOPE)

# Add RNG test executable
add_executable(clo_rng_test clo_rng_test.c clo_rng.c)

# Set RNG test executable dependencies
target_link_libraries(clo_rng_test clo_common)

# Set RNG test executable version
set_target_properties(clo_rng_test PROPERTIES VERSION ${${PROJECT_NAME}_VERSION})

# Copy OpenCL kernels to the same location as the RNG executable
add_custom_command(TARGET clo_rng POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
	${CMAKE_CURRENT_SOURCE_DIR}/*.cl
	$<TARGET_FILE_DIR:clo_rng_test>
)

# Where to install RNG test executable
install(TARGETS clo_rng_test RUNTIME DESTINATION bin)

# Where to install OpenCL kernels
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
	DESTINATION share/cl
	FILES_MATCHING PATTERN "*.cl")

# Where to install headers
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/clo_rng.h
	DESTINATION include/${PROJECT_NAME})

