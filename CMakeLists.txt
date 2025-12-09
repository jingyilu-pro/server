# cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/work/app/bin ..
# cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/work/app/bin ..

# CMake
cmake_minimum_required(VERSION 3.28)

Project(application)

# find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -levent_core -lhiredis -ljansson -ljsoncpp -- -lprotobuf -lutf8_range -lutf8_validity
# set(MAIN_LIB_DIR "-ldl -lpthread")

# 安装目录
set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/bin")

MESSAGE(STATUS " CMAKE_BUILD_TYPE = " ${CMAKE_BUILD_TYPE})
MESSAGE(STATUS " 运行目录 = " ${CMAKE_INSTALL_PREFIX})
MESSAGE(STATUS " 工程目录 = " ${PROJECT_SOURCE_DIR})

# 依赖库
include("cmake/dependencise_libs.cmake")
# 通用库
set(COMMON_LIB "common")
# 协议库
set(PROTOCOL_LIB "protocol")
# 基础服务库
set(SERVICE_BASE_DIR "servicebase")
# 服务库
set(SERVICE_LIB "maskword_service")

if( CMAKE_BUILD_TYPE STREQUAL "Release" )
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -std=c99 -fPIC -g -O2 -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++2a -g3 -O2 -Wall -fcoroutines -fpermissive -D__CONCURRENTQUEUE__")
else()
    set(CMAKE_BUILD_TYPE "Debug")
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -std=c99 -O2 -Wall -g -ggdb -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -O2 -Wall -g3 -ggdb -fPIC -fcoroutines -fpermissive -D__CONCURRENTQUEUE__") #-D__APPLE__  -D__CONCURRENTQUEUE__
endif()


# main logic
SET(SERVICE_DIR "${PROJECT_SOURCE_DIR}/app/service")
include_directories(
    # common
    ${PROJECT_SOURCE_DIR}/common
    ${PROJECT_SOURCE_DIR}/common/coroutine
    ${PROJECT_SOURCE_DIR}/common/utils

    # app protocol
    ${PROJECT_SOURCE_DIR}/app/protocol

    # service
    ${SERVICE_DIR}/base
    ${SERVICE_DIR}/maskword/common
    ${SERVICE_DIR}/maskword/logic
)

# common lib
add_subdirectory(${PROJECT_SOURCE_DIR}/common)

# proto lib
add_subdirectory(${PROJECT_SOURCE_DIR}/app/protocol)

# serice lib
add_subdirectory(${SERVICE_DIR}/base)
add_subdirectory(${SERVICE_DIR}/maskword)

# app
add_subdirectory(${PROJECT_SOURCE_DIR}/app/application)
add_subdirectory(${PROJECT_SOURCE_DIR}/app/apptool)

