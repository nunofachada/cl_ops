# Add cl-ops subdir
add_subdirectory(cl_ops)

# Add benchmarks subdir
add_subdirectory(benchmarks)

# Add tests subdir
add_subdirectory(tests)

# Where to install aggregate header
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cl_ops.h
	DESTINATION include)


