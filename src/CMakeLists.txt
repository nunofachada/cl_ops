# Subdirectories to process
#set(CLO_SUBDIRS common rng sort scan)
set(CLO_SUBDIRS common rng scan)

# Destination for the OpenCL kernels
set(CLO_CL_DEST_SRC ${CMAKE_CURRENT_BINARY_DIR}/${CLO_CL_DEST})

# Sources for the aggregated cl-ops shared library, initially empty
set(CLO_LIB_SRCS "")

# Process subdirectories
foreach(CLO_SUBDIR ${CLO_SUBDIRS})

	# Reset sources for current subdirectory
	set(CLO_LIB_SRCS_CURRENT "")

	# Add current subdirectory (will set the CLO_LIB_SRCS_CURRENT variable)
	add_subdirectory(${CLO_SUBDIR})

	# Add sources from current subdirectory
	foreach(CLO_LIB_SRC_CURRENT ${CLO_LIB_SRCS_CURRENT})
		set(CLO_LIB_SRCS ${CLO_LIB_SRCS} ${CLO_SUBDIR}/${CLO_LIB_SRC_CURRENT})
	endforeach()

endforeach()


# Add cl-ops library
add_library(cl_ops SHARED ${CLO_LIB_SRCS})

# Set cl-ops library dependencies
target_link_libraries(cl_ops ${OPENCL_LIBRARIES} ${GLIB_LIBRARIES} ${GLIB_LDFLAGS} ${CF4OCL2_LIBRARIES})

# Set library version
set_target_properties(cl_ops PROPERTIES
	VERSION ${${PROJECT_NAME}_VERSION}
	SOVERSION ${${PROJECT_NAME}_VERSION_MAJOR})

# Where to install lib
install(TARGETS cl_ops
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	RUNTIME DESTINATION bin)

# Where to install aggregate header
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/cl_ops.h
	DESTINATION include)


