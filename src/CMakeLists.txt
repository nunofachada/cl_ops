# Add cl-ops subdir
add_subdirectory(cl_ops)

# Add tests subdir
add_subdirectory(tests)

# Where to install aggregate header
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cl_ops.h
	DESTINATION include)


