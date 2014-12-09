# Add RNG source to aggregated library sources list
set(CLO_LIB_SRCS_CURRENT clo_rng.c PARENT_SCOPE)

set(RNG_SRCS api init workitem lcg xorshift64 xorshift128 mwc64x parkmiller tauslcg)

foreach(RNG_SRC ${RNG_SRCS})

	string(TOUPPER ${RNG_SRC} UP)
	file(READ ${CMAKE_CURRENT_SOURCE_DIR}/clo_rng_${RNG_SRC}.cl RNG_SRC_${UP}_RAW HEX)
	string(REGEX REPLACE "(..)" "\\\\x\\1" RNG_SRC_${UP} ${RNG_SRC_${UP}_RAW})

endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_rng.in.h
	${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_rng.h @ONLY)

# Install the configured header
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}/clo_rng.h
	DESTINATION ${INSTALL_SUBDIR_INCLUDE}/${PROJECT_NAME})
