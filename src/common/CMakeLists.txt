# Add common library source to aggregated library sources list
set(CLO_LIB_SRCS_CURRENT clo_common.c PARENT_SCOPE)

# Add common library
add_library(clo_common STATIC clo_common.c)

# Set common library dependencies
target_link_libraries(clo_common ${OPENCL_LIBRARIES} ${GLIB_LIBRARIES} ${CF4OCL_LIBRARIES})

# Where to install headers
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/clo_common.h
	DESTINATION include/${PROJECT_NAME})
	
	
