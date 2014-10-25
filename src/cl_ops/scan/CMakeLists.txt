# Add Scan source to aggregated library sources list
set(CLO_LIB_SRCS_CURRENT clo_scan_abstract.c clo_scan_blelloch.c
	PARENT_SCOPE)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/clo_scan_blelloch.cl
	BLELLOCH_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" BLELLOCH_SRC ${BLELLOCH_SRC_RAW})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_scan_blelloch.in.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_scan_blelloch.h @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_scan_abstract.in.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_scan_abstract.h @ONLY)

# Install the configured headers
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_scan_abstract.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_scan_blelloch.h
	DESTINATION ${INSTALL_SUBDIR_INCLUDE}/${PROJECT_NAME})

