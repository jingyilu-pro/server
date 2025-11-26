# cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/work/app/bin ..
# cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/work/app/bin ..

# CMake
cmake_minimum_required(VERSION 3.28)

Project(application)

# https://cmake.org/cmake/help/latest/module/ExternalProject.html
include(ExternalProject)

# find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 运行目录
set(SOLUTION_DIR "${PROJECT_SOURCE_DIR}")
set(THIRD_DIR "${PROJECT_SOURCE_DIR}/libs")

# -levent_core -lhiredis -ljansson -ljsoncpp
set(THIRD_LIB_DIR "-ldl -lpthread -lprotobuf -lutf8_range -lutf8_validity")
# 运行目录
set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/bin")

MESSAGE(STATUS " CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})
MESSAGE(STATUS " 运行目录 = " ${CMAKE_INSTALL_PREFIX})
MESSAGE(STATUS " 工程目录 = " ${SOLUTION_DIR})


include("cmake/dependencise_libs.cmake")
build_dependencies()
link_dependencies("BEFORE")


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


