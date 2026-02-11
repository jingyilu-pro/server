# https://cmake.org/cmake/help/latest/module/ExternalProject.html
include(ExternalProject)

# Set options for LibEvent, disable all their tests and benchmarks:
# set(EVENT__DISABLE_OPENSSL   YES CACHE BOOL   "Disable OpenSSL in LibEvent")
# set(EVENT__DISABLE_BENCHMARK YES CACHE BOOL   "Disable LibEvent benchmarks")
# set(EVENT__DISABLE_TESTS     YES CACHE BOOL   "Disable LibEvent tests")
# set(EVENT__DISABLE_REGRESS   YES CACHE BOOL   "Disable LibEvent regression tests")
# set(EVENT__DISABLE_SAMPLES   YES CACHE BOOL   "Disable LibEvent samples")
# set(EVENT__LIBRARY_TYPE "STATIC" CACHE STRING "Use static LibEvent libraries")

# Build libevent as an external project.
set(LIBEVENT_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/libevent)
ExternalProject_Add(libevent_external_project
					SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/libevent
					PREFIX      ${LIBEVENT_INSTALL_DIR}
					INSTALL_DIR ${LIBEVENT_INSTALL_DIR}
					CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${LIBEVENT_INSTALL_DIR}
								-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
								-DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
				)
include_directories(BEFORE SYSTEM ${LIBEVENT_INSTALL_DIR}/include)
link_directories(${LIBEVENT_INSTALL_DIR}/lib)

# Build fmt as an external project.
set(FMT_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/fmt)
ExternalProject_Add(fmt_external_project
					SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/fmt
					PREFIX      ${FMT_INSTALL_DIR}
					INSTALL_DIR ${FMT_INSTALL_DIR}
					CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${FMT_INSTALL_DIR}
								-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
								-DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
				)
include_directories(BEFORE SYSTEM ${FMT_INSTALL_DIR}/include)
link_directories(${FMT_INSTALL_DIR}/lib)

# Build concurrentqueue as an external project.
set(CONCURRENTQUEUE_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/concurrentqueue)
ExternalProject_Add(concurrentqueue_external_project
					SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/concurrentqueue
					PREFIX      ${CONCURRENTQUEUE_INSTALL_DIR}
					INSTALL_DIR ${CONCURRENTQUEUE_INSTALL_DIR}
					CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${CONCURRENTQUEUE_INSTALL_DIR}
								-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
								-DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
				)
include_directories(BEFORE SYSTEM ${CONCURRENTQUEUE_INSTALL_DIR}/include/concurrentqueue/moodycamel)
link_directories(${CONCURRENTQUEUE_INSTALL_DIR}/lib)

# Build readerwriterqueue as an external project.
set(READERWRITERQUEUE_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/readerwriterqueue)
ExternalProject_Add(readerwriterqueue_external_project
					SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/readerwriterqueue
					PREFIX      ${READERWRITERQUEUE_INSTALL_DIR}
					INSTALL_DIR ${READERWRITERQUEUE_INSTALL_DIR}
					CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${READERWRITERQUEUE_INSTALL_DIR}
								-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
								-DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
				)
include_directories(BEFORE SYSTEM ${READERWRITERQUEUE_INSTALL_DIR}/include)
link_directories(${READERWRITERQUEUE_INSTALL_DIR}/lib)


# Build curl as an external project.
set(CURL_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/curl)
ExternalProject_Add(curl_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/curl
                    PREFIX      ${CURL_INSTALL_DIR}
                    INSTALL_DIR ${CURL_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${CURL_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                                # -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
                                -DBUILD_SHARED_LIBS=OFF
                                -DBUILD_STATIC_LIBS=ON
                                -DBUILD_CURL_EXE=OFF
                                -DCURL_ENABLE_SSL=OFF
                   )
include_directories(BEFORE SYSTEM ${CURL_INSTALL_DIR}/include)
link_directories(${CURL_INSTALL_DIR}/lib)


# Build hiredis as an external project.
set(HIREDIS_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/hiredis)
ExternalProject_Add(hiredis_external_project
					SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/hiredis
					PREFIX      ${HIREDIS_INSTALL_DIR}
					INSTALL_DIR ${HIREDIS_INSTALL_DIR}
					CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${HIREDIS_INSTALL_DIR}
								-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
								-DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
				)
include_directories(BEFORE SYSTEM ${HIREDIS_INSTALL_DIR}/include)
link_directories(${HIREDIS_INSTALL_DIR}/lib)

# Build mariadb-connector-c as an external project.
set(MARIADB_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/mariadb-connector-c)
ExternalProject_Add(mariadb_external_project
					SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/mariadb-connector-c
					PREFIX      ${MARIADB_INSTALL_DIR}
					INSTALL_DIR ${MARIADB_INSTALL_DIR}
					CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${MARIADB_INSTALL_DIR}
								-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
								-DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
				)
include_directories(BEFORE SYSTEM ${MARIADB_INSTALL_DIR}/include)
link_directories(${MARIADB_INSTALL_DIR}/lib)
link_directories(${MARIADB_INSTALL_DIR}/lib/mariadb)

# Build spdlog as an external project.
set(SPDLOG_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/spdlog)
ExternalProject_Add(spdlog_external_project
					SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/spdlog
					PREFIX      ${SPDLOG_INSTALL_DIR}
					INSTALL_DIR ${SPDLOG_INSTALL_DIR}
					CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${SPDLOG_INSTALL_DIR}
								-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
								-DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
				)
include_directories(BEFORE SYSTEM ${SPDLOG_INSTALL_DIR}/include)
link_directories(${SPDLOG_INSTALL_DIR}/lib)
list(APPEND CMAKE_PREFIX_PATH ${SPDLOG_INSTALL_DIR}/lib/cmake)


# Build protobuf as an external project.
set(PROTOBUF_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/protobuf)
ExternalProject_Add(protobuf_external_project
					SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/protobuf
					PREFIX      ${PROTOBUF_INSTALL_DIR}
					INSTALL_DIR ${PROTOBUF_INSTALL_DIR}
					CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${PROTOBUF_INSTALL_DIR}
								-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
								-DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
								# -Dprotobuf_BUILD_SHARED_LIBS=OFF
								-Dprotobuf_BUILD_TESTS=OFF
								-DABSL_ENABLE_INSTALL=ON
								# -Dprotobuf_LOCAL_DEPENDENCIES_ONLY=ON
					)
include_directories(BEFORE SYSTEM ${PROTOBUF_INSTALL_DIR}/include)
link_directories(${PROTOBUF_INSTALL_DIR}/lib)
list(APPEND CMAKE_PREFIX_PATH ${PROTOBUF_INSTALL_DIR}/lib/cmake)

# Build mbedtls as an external project.
set(MBEDTLS_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/mbedtls)
ExternalProject_Add(mbedtls_external_project
					SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/mbedtls
					PREFIX      ${MBEDTLS_INSTALL_DIR}
					INSTALL_DIR ${MBEDTLS_INSTALL_DIR}
					CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${MBEDTLS_INSTALL_DIR}
								-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
								-DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
								-DENABLE_PROGRAMS=OFF
								-DENABLE_TESTING=OFF
					)
include_directories(BEFORE SYSTEM ${MBEDTLS_INSTALL_DIR}/include)
link_directories(${MBEDTLS_INSTALL_DIR}/lib)
# list(APPEND CMAKE_PREFIX_PATH ${MBEDTLS_INSTALL_DIR}/lib/cmake)

# Build libzmq as an external project.
# set(LIBZMQ_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/libzmq)
# ExternalProject_Add(libzmq_external_project
#                     SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/libzmq
#                     PREFIX      ${LIBZMQ_INSTALL_DIR}
#                     INSTALL_DIR ${LIBZMQ_INSTALL_DIR}
#                     CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${LIBZMQ_INSTALL_DIR}
#                                 -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
#                                 -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
#                    )
# include_directories(BEFORE SYSTEM ${LIBZMQ_INSTALL_DIR}/include)
# link_directories(${LIBZMQ_INSTALL_DIR}/lib)


# Build libco as an external project.
# set(LIBCO_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/libco)
# ExternalProject_Add(libco_external_project
#                     SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/libco
#                     PREFIX      ${LIBCO_INSTALL_DIR}
#                     INSTALL_DIR ${LIBCO_INSTALL_DIR}
#                     CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${LIBCO_INSTALL_DIR}
#                                 -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
#                                 -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
#                    )
# include_directories(BEFORE SYSTEM ${LIBCO_INSTALL_DIR}/include)
# link_directories(${LIBCO_INSTALL_DIR}/lib)

# Build libjwt as an external project.
set(LIBJWT_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/libjwt)
ExternalProject_Add(libjwt_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/libjwt
                    PREFIX      ${LIBJWT_INSTALL_DIR}
                    INSTALL_DIR ${LIBJWT_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${LIBJWT_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
								-DWITH_GNUTLS=OFF
								# -DCMAKE_MODULE_PATH:STRING=${JANSSON_CMAKE_MODULE_PATH}  # 传递模块路径
								# -DCMAKE_PREFIX_PATH:STRING=${JANSSON_CMAKE_PREFIX_PATH}  # 传递模块路径
                    # DEPENDS     jansson_external_project
                   )
include_directories(BEFORE SYSTEM ${LIBJWT_INSTALL_DIR}/include)
link_directories(${LIBJWT_INSTALL_DIR}/lib)

# Build recastnavigation as an external project.
set(RECASTNAVIGATION_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/recastnavigation)
ExternalProject_Add(recastnavigation_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/recastnavigation
                    PREFIX      ${RECASTNAVIGATION_INSTALL_DIR}
                    INSTALL_DIR ${RECASTNAVIGATION_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${RECASTNAVIGATION_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
								-DRECASTNAVIGATION_DEMO=OFF
								-DRECASTNAVIGATION_TESTS=OFF
                   )
include_directories(BEFORE SYSTEM ${RECASTNAVIGATION_INSTALL_DIR}/include)
link_directories(${RECASTNAVIGATION_INSTALL_DIR}/lib)
set(DEPENDENCIES_LIBS ${DEPENDENCIES_LIBS} -lDetour -lDetourCrowd -lDetourTileCache -lRecast)

# Build tinyxml2 as an external project.
set(TINYXML_INSTALL_DIR ${CMAKE_BINARY_DIR}/libs/tinyxml2)
ExternalProject_Add(tinyxml2_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/libs/tinyxml2
                    PREFIX      ${TINYXML_INSTALL_DIR}
                    INSTALL_DIR ${TINYXML_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${TINYXML_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                   )
include_directories(BEFORE SYSTEM ${TINYXML_INSTALL_DIR}/include)
link_directories(${TINYXML_INSTALL_DIR}/lib)
set(DEPENDENCIES_LIBS ${DEPENDENCIES_LIBS} -ltinyxml2)

set(DEPENDENCIES_LIBS ${DEPENDENCIES_LIBS} -levent -levent_core -levent_extra -levent_pthreads)
set(DEPENDENCIES_LIBS ${DEPENDENCIES_LIBS} -lhiredis -lmariadb -ljwt -lssl -lcrypto -ljansson)


# ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
# 设置 CMAKE_PREFIX_PATH
# list(APPEND CMAKE_PREFIX_PATH ${PROTOBUF_INSTALL_DIR}/lib/cmake)
# set(CMAKE_PREFIX_PATH ${PROTOBUF_INSTALL_DIR}/lib/cmake)
# find_package(Protobuf REQUIRED PATHS  ${PROTOBUF_INSTALL_DIR}/lib/cmake NO_DEFAULT_PATH)

# # Find required protobuf package
find_package(protobuf CONFIG REQUIRED)
find_package(absl CONFIG)
set(ABSEL_LIBARARY
	absl::absl_check
	absl::absl_log
	absl::algorithm
	absl::base
	absl::bind_front
	absl::bits
	absl::btree
	absl::cleanup
	absl::cord
	absl::core_headers
	absl::debugging
	absl::die_if_null
	absl::dynamic_annotations
	absl::flags
	absl::flat_hash_map
	absl::flat_hash_set
	absl::function_ref
	absl::hash
	absl::layout
	absl::log_initialize
	absl::log_globals
	absl::log_severity
	absl::memory
	absl::node_hash_map
	absl::node_hash_set
	absl::random_distributions
	absl::random_random
	absl::span
	absl::status
	absl::statusor
	absl::strings
	absl::synchronization
	absl::time
	absl::type_traits
	absl::utility
	)


# 最后把要连接的库增加到这里
set(DEPENDENCIES_LIBS  ${DEPENDENCIES_LIBS} -lprotobuf -lutf8_range -lutf8_validity ${ABSEL_LIBARARY})
