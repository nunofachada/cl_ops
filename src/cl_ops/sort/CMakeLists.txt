# Add sort source to aggregated library sources list
set(CLO_LIB_SRCS_CURRENT clo_sort_abstract.c clo_sort_sbitonic.c
	PARENT_SCOPE)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/clo_sort_sbitonic.cl
	SBITONIC_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" SBITONIC_SRC ${SBITONIC_SRC_RAW})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_sort_abstract.in.h
	${CMAKE_BINARY_DIR}/cl_ops/clo_sort_abstract.h @ONLY)




