# Add sort source to aggregated library sources list
set(CLO_LIB_SRCS_CURRENT clo_sort_abstract.c clo_sort_sbitonic.c
	clo_sort_gselect.c clo_sort_abitonic.c clo_sort_satradix.c
	PARENT_SCOPE)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/clo_sort_sbitonic.cl
	SBITONIC_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" SBITONIC_SRC ${SBITONIC_SRC_RAW})

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/clo_sort_gselect.cl
	GSELECT_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" GSELECT_SRC ${GSELECT_SRC_RAW})

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/clo_sort_abitonic.cl
	ABITONIC_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" ABITONIC_SRC ${ABITONIC_SRC_RAW})

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/clo_sort_satradix.cl
	SATRADIX_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" SATRADIX_SRC ${SATRADIX_SRC_RAW})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_sort_abstract.in.h
	${CMAKE_BINARY_DIR}/cl_ops/clo_sort_abstract.h @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_sort_sbitonic.in.h
	${CMAKE_BINARY_DIR}/cl_ops/clo_sort_sbitonic.h @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_sort_gselect.in.h
	${CMAKE_BINARY_DIR}/cl_ops/clo_sort_gselect.h @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_sort_abitonic.in.h
	${CMAKE_BINARY_DIR}/cl_ops/clo_sort_abitonic.h @ONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_sort_satradix.in.h
	${CMAKE_BINARY_DIR}/cl_ops/clo_sort_satradix.h @ONLY)

# Install the configured headers
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_sort_abstract.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_sort_sbitonic.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_sort_gselect.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_sort_abitonic.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_sort_satradix.h
	DESTINATION ${INSTALL_SUBDIR_INCLUDE}/${PROJECT_NAME})
