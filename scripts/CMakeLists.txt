# Available scripts
set(SCRIPTS clo_rng_plot.py)

# Where to install scripts
foreach(SCRIPT ${SCRIPTS})
	install(PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/${SCRIPT}
		DESTINATION bin)
endforeach()


