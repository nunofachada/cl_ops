# Sort implementations
set(CLO_SORT_IMPL "sort" "sort_sbitonic")

# Sort implementations source
foreach(impl ${CLO_SORT_IMPL})
	# Add .c suffix to implementation name
	set(CLO_SORT_IMPL_SRC ${CLO_SORT_IMPL_SRC} clo_${impl}.c)
	# Where to install correspondent implementation header file
	install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/clo_${impl}.h
		DESTINATION include/${PROJECT_NAME})	
endforeach()

# Add sort sources to aggregated library sources list
set(CLO_LIB_SRCS_CURRENT ${CLO_SORT_IMPL_SRC} PARENT_SCOPE)

# Add sort test executable
add_executable(clo_sort_test clo_sort_test.c ${CLO_SORT_IMPL_SRC})

# Set sort test executable dependencies
target_link_libraries(clo_sort_test clo_common)

# Set sort test executable version
set_target_properties(clo_sort_test PROPERTIES VERSION ${${PROJECT_NAME}_VERSION})

# Copy OpenCL kernels to a location where they can be invoked by the
# sort test executable without installation
file(GLOB CL_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cl)
foreach(CL_FILE ${CL_FILES})
	add_custom_command(TARGET clo_sort_test POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${CL_FILE}
		${CLO_CL_DEST_SRC}/
	)
endforeach()

# Where to install sort test executable
install(TARGETS clo_sort_test RUNTIME DESTINATION bin)

# Where to install OpenCL kernels
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
	DESTINATION share/${PROJECT_NAME}
	FILES_MATCHING PATTERN "*.cl")
