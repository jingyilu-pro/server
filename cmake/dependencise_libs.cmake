function(build_dependencies)
	# Set options for LibEvent, disable all their tests and benchmarks:
	set(EVENT__DISABLE_OPENSSL   YES CACHE BOOL   "Disable OpenSSL in LibEvent")
	set(EVENT__DISABLE_BENCHMARK YES CACHE BOOL   "Disable LibEvent benchmarks")
	set(EVENT__DISABLE_TESTS     YES CACHE BOOL   "Disable LibEvent tests")
	set(EVENT__DISABLE_REGRESS   YES CACHE BOOL   "Disable LibEvent regression tests")
	set(EVENT__DISABLE_SAMPLES   YES CACHE BOOL   "Disable LibEvent samples")
	set(EVENT__LIBRARY_TYPE "STATIC" CACHE STRING "Use static LibEvent libraries")

	# Set options for JsonCPP, disabling all of their tests:
	# set(JSONCPP_WITH_TESTS OFF CACHE BOOL "Compile and (for jsoncpp_check) run JsonCpp test executables")
	# set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "Automatically run unit-tests as a post build step")
	# set(JSONCPP_WITH_PKGCONFIG_SUPPORT OFF CACHE BOOL "Generate and install .pc files")
	# set(JSONCPP_WITH_CMAKE_PACKAGE OFF CACHE BOOL "Generate and install cmake package files")
	# set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build jsoncpp_lib as a shared library.")
	# set(BUILD_OBJECT_LIBS OFF CACHE BOOL "Build jsoncpp_lib as a object library.")

	# Set options for mbedtls:
	set(ENABLE_PROGRAMS OFF CACHE BOOL "Build mbed TLS programs.")
	set(ENABLE_TESTING OFF CACHE BOOL "Build mbed TLS tests.")

	# Enumerate all submodule libraries
	set(DEPENDENCIES mbedtls fmt libevent concurrentqueue)
	foreach(DEPENDENCY ${DEPENDENCIES})
		# Check that the libraries are present:
		if (NOT EXISTS "${PROJECT_SOURCE_DIR}/libs/${DEPENDENCY}/CMakeLists.txt")
			message(FATAL_ERROR "${DEPENDENCY} is missing in folder libs/${DEPENDENCY}. Have you initialized and updated the submodules / downloaded the extra libraries?")
		endif()

		# Include all the libraries
		# We use EXCLUDE_FROM_ALL so that only the explicit dependencies are compiled
		# (mbedTLS also has test and example programs in their CMakeLists.txt, we don't want those):
		add_subdirectory("libs/${DEPENDENCY}" EXCLUDE_FROM_ALL)
	endforeach()

endfunction()

function(link_dependencies)
	# Add required includes:
	include_directories (
		"BEFORE" SYSTEM PRIVATE
		libs/mbedtls/include
		# libs/TCLAP/include
		libs # TODO fix files including zlib/x instead of x
		libs/concurrentqueue
	)

	# include_directories(BEFORE SYSTEM ${READERWRITERQUEUE_INSTALL_DIR}/include)

	# Link dependencies as private:
	# target_link_libraries(
	# 	${TARGET} PRIVATE
	# 	event_core
	# 	event_extra
	# 	fmt::fmt
	# 	concurrentqueue
	# 	# mbedtls
	# )

endfunction()
