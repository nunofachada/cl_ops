# Add common library source to aggregated library sources list
set(CLO_LIB_SRCS_CURRENT clo_common.c PARENT_SCOPE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_common.in.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_common.h @ONLY)

# Install the configured header
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_common.h
	DESTINATION ${INSTALL_SUBDIR_INCLUDE}/${PROJECT_NAME})
