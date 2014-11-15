# List of benchmarks
set(BENCHMARKS clo_scan_bench clo_rng_bench clo_sort_bench)

# Add current folder as an include folder
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Add a target for each utility
foreach(BENCHMARK ${BENCHMARKS})
	add_executable(${BENCHMARK} ${BENCHMARK}.c clo_bench.c)
	target_link_libraries(${BENCHMARK} ${PROJECT_NAME}
		${OPENCL_LIBRARIES} ${GLIB_LIBRARIES} ${GLIB_LDFLAGS}
		${CF4OCL2_LIBRARIES})
endforeach()

# Benchmarks requiring additional OCL code
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/clo_rng_bench.cl RNG_BENCHMARK_SRC_RAW HEX)
string(REGEX REPLACE "(..)" "\\\\x\\1" RNG_BENCHMARK_SRC ${RNG_BENCHMARK_SRC_RAW})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/clo_rng_bench.in.h
	${CMAKE_BINARY_DIR}/clo_rng_bench.h @ONLY)

# How to install benchmarks
install(TARGETS ${BENCHMARKS} RUNTIME DESTINATION bin)

# Add a target which builds all benchmarks
add_custom_target(benchmarks DEPENDS ${BENCHMARKS})

