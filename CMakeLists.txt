# cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/work/app/bin ..
# cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/work/app/bin ..

# CMake
cmake_minimum_required(VERSION 3.28)

Project(server)

# https://cmake.org/cmake/help/latest/module/ExternalProject.html
include(ExternalProject)

# find_package(OpenSSL REQUIRED)

# 运行目录
set(SOLUTION_DIR "${PROJECT_SOURCE_DIR}")
set(THIRD_DIR "${PROJECT_SOURCE_DIR}/third")

# -levent_core -lhiredis -ljansson -ljsoncpp
set(THIRD_LIB_DIR "-ldl -lpthread -lprotobuf -lutf8_range -lutf8_validity")
# 运行目录
set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/bin")

MESSAGE(STATUS " CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})
MESSAGE(STATUS " 运行目录 = " ${CMAKE_INSTALL_PREFIX})
MESSAGE(STATUS " 工程目录 = " ${SOLUTION_DIR})

set(COMMON_LIB "common")
set(PROTOCOL_LIB "protocol")
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

if( CMAKE_BUILD_TYPE STREQUAL "Release" )
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -std=c99 -fPIC -g -O2 -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++2a -g -O2 -Wall -fcoroutines -fpermissive -D__CONCURRENTQUEUE__")
else()
    set(CMAKE_BUILD_TYPE "Debug")
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -std=c99 -O0 -Wall -g -ggdb -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -O0 -Wall -g -ggdb -fPIC -fcoroutines -fpermissive -D__CONCURRENTQUEUE__") #-D__APPLE__  -D__CONCURRENTQUEUE__
endif()


# Build libevent as an external project.
set(LIBEVENT_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/libevent)
ExternalProject_Add(libevent_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/libevent
                    PREFIX      ${LIBEVENT_INSTALL_DIR}
                    INSTALL_DIR ${LIBEVENT_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${LIBEVENT_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                    # DEPENDS     jansson_external_project
                   )
include_directories(BEFORE SYSTEM ${LIBEVENT_INSTALL_DIR}/include)
link_directories(${LIBEVENT_INSTALL_DIR}/lib)


# Build concurrentqueue as an external project.
set(CONCURRENTQUEUE_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/concurrentqueue)
ExternalProject_Add(concurrentqueue_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/concurrentqueue
                    PREFIX      ${CONCURRENTQUEUE_INSTALL_DIR}
                    INSTALL_DIR ${CONCURRENTQUEUE_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${CONCURRENTQUEUE_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                    # DEPENDS     jansson_external_project
                   )
include_directories(BEFORE SYSTEM ${CONCURRENTQUEUE_INSTALL_DIR}/include/concurrentqueue/moodycamel)
link_directories(${CONCURRENTQUEUE_INSTALL_DIR}/lib)

# Build readerwriterqueue as an external project.
set(READERWRITERQUEUE_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/readerwriterqueue)
ExternalProject_Add(readerwriterqueue_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/readerwriterqueue
                    PREFIX      ${READERWRITERQUEUE_INSTALL_DIR}
                    INSTALL_DIR ${READERWRITERQUEUE_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${READERWRITERQUEUE_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                   )
include_directories(BEFORE SYSTEM ${READERWRITERQUEUE_INSTALL_DIR}/include)
link_directories(${READERWRITERQUEUE_INSTALL_DIR}/lib)


# Build hiredis as an external project.
set(HIREDIS_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/hiredis)
ExternalProject_Add(hiredis_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/hiredis
                    PREFIX      ${HIREDIS_INSTALL_DIR}
                    INSTALL_DIR ${HIREDIS_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${HIREDIS_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                   )
include_directories(BEFORE SYSTEM ${HIREDIS_INSTALL_DIR}/include)
link_directories(${HIREDIS_INSTALL_DIR}/lib)

# Build mariadb-connector-c as an external project.
set(MARIADB_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/mariadb-connector-c)
ExternalProject_Add(mariadb_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/mariadb-connector-c
                    PREFIX      ${MARIADB_INSTALL_DIR}
                    INSTALL_DIR ${MARIADB_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${MARIADB_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                   )
include_directories(BEFORE SYSTEM ${MARIADB_INSTALL_DIR}/include)
link_directories(${MARIADB_INSTALL_DIR}/lib)

# Build spdlog as an external project.
set(SPDLOG_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/spdlog)
ExternalProject_Add(spdlog_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/spdlog
                    PREFIX      ${SPDLOG_INSTALL_DIR}
                    INSTALL_DIR ${SPDLOG_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${SPDLOG_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                   )
include_directories(BEFORE SYSTEM ${SPDLOG_INSTALL_DIR}/include)
link_directories(${SPDLOG_INSTALL_DIR}/lib)

# # Build protobuf as an external project.
# set(PROTOBUF_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/protobuf)
# ExternalProject_Add(protobuf_external_project
#                     SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/protobuf
#                     PREFIX      ${PROTOBUF_INSTALL_DIR}
#                     INSTALL_DIR ${PROTOBUF_INSTALL_DIR}
#                     CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${PROTOBUF_INSTALL_DIR}
#                                 -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
#                                 -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
#                                 # -Dprotobuf_BUILD_SHARED_LIBS=OFF
#                                 -Dprotobuf_BUILD_TESTS=OFF
#                                 -DABSL_ENABLE_INSTALL=ON
#                                 # -Dprotobuf_LOCAL_DEPENDENCIES_ONLY=ON
#                    )
# include_directories(BEFORE SYSTEM ${PROTOBUF_INSTALL_DIR}/include)
# link_directories(${PROTOBUF_INSTALL_DIR}/lib)

# Build libzmq as an external project.
set(LIBZMQ_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/libzmq)
ExternalProject_Add(libzmq_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/libzmq
                    PREFIX      ${LIBZMQ_INSTALL_DIR}
                    INSTALL_DIR ${LIBZMQ_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${LIBZMQ_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                   )
include_directories(BEFORE SYSTEM ${LIBZMQ_INSTALL_DIR}/include)
link_directories(${LIBZMQ_INSTALL_DIR}/lib)

# Build curl as an external project.
set(CURL_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/curl)
ExternalProject_Add(curl_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/curl
                    PREFIX      ${CURL_INSTALL_DIR}
                    INSTALL_DIR ${CURL_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${CURL_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                   )
include_directories(BEFORE SYSTEM ${CURL_INSTALL_DIR}/include)
link_directories(${CURL_INSTALL_DIR}/lib)


# Build libco as an external project.
# set(LIBCO_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/libco)
# ExternalProject_Add(libco_external_project
#                     SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/libco
#                     PREFIX      ${LIBCO_INSTALL_DIR}
#                     INSTALL_DIR ${LIBCO_INSTALL_DIR}
#                     CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${LIBCO_INSTALL_DIR}
#                                 -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
#                                 -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
#                    )
# include_directories(BEFORE SYSTEM ${LIBCO_INSTALL_DIR}/include)
# link_directories(${LIBCO_INSTALL_DIR}/lib)

# Build jansson as an external project.
# set(JANSSON_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/jansson)
# ExternalProject_Add(jansson_external_project
#                     SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/jansson
#                     PREFIX      ${JANSSON_INSTALL_DIR}
#                     INSTALL_DIR ${JANSSON_INSTALL_DIR}
#                     CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${JANSSON_INSTALL_DIR}
#                                 -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
#                                 -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
#                    )
# include_directories(BEFORE SYSTEM ${JANSSON_INSTALL_DIR}/include)
# link_directories(${JANSSON_INSTALL_DIR}/lib)

# Build libjwt as an external project.
set(LIBJWT_INSTALL_DIR ${CMAKE_BINARY_DIR}/third/libjwt)
ExternalProject_Add(libjwt_external_project
                    SOURCE_DIR  ${CMAKE_SOURCE_DIR}/third/libjwt
                    PREFIX      ${LIBJWT_INSTALL_DIR}
                    INSTALL_DIR ${LIBJWT_INSTALL_DIR}
                    CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX:PATH=${LIBJWT_INSTALL_DIR}
                                -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                                -DCMAKE_CXX_FLAGS=${EXTERNAL_PROJECT_CMAKE_CXX_FLAGS}
                    # DEPENDS     jansson_external_project
                   )
include_directories(BEFORE SYSTEM ${LIBJWT_INSTALL_DIR}/include)
link_directories(${LIBJWT_INSTALL_DIR}/lib)




# main logic
include_directories(
    # common
    ${SOLUTION_DIR}/common
    ${SOLUTION_DIR}/common/coroutine
    ${SOLUTION_DIR}/common/utils

    # app protocol
    ${SOLUTION_DIR}/app/protocol

    # service
    ${SOLUTION_DIR}/app/service/base
    ${SOLUTION_DIR}/app/service/maskword/common
    ${SOLUTION_DIR}/app/service/maskword/logic
)

# common lib
add_subdirectory(${SOLUTION_DIR}/common)

# proto lib
add_subdirectory(${SOLUTION_DIR}/app/protocol)

# serice lib
add_subdirectory(${SOLUTION_DIR}/app/service/base)
add_subdirectory(${SOLUTION_DIR}/app/service/maskword)

# app
add_subdirectory(${SOLUTION_DIR}/app/application)

