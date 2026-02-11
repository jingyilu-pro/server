# cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/work/app/bin ..
# cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/work/app/bin ..

cmake_minimum_required(VERSION 3.28)

project(application)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/bin")

message(STATUS " CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message(STATUS " 运行目录 = ${CMAKE_INSTALL_PREFIX}")
message(STATUS " 工程目录 = ${PROJECT_SOURCE_DIR}")

include("cmake/dependencise_libs.cmake")

set(COMMON_LIB "common")
set(PROTOCOL_LIB "protocol")
set(SERVICE_BASE_DIR "servicebase")
set(SERVICE_LIB
    maskword_service
    server_service
    client_pressure_service
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -fPIC -g -O2 -Wall")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++2a -g3 -O2 -Wall -fcoroutines -fpermissive -D__CONCURRENTQUEUE__")
else()
    set(CMAKE_BUILD_TYPE "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O2 -Wall -g -ggdb -fPIC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -O2 -Wall -g3 -ggdb -fPIC -fcoroutines -fpermissive -D__CONCURRENTQUEUE__")
endif()

set(SERVICE_DIR "${PROJECT_SOURCE_DIR}/app/service")

include_directories(
    ${PROJECT_SOURCE_DIR}/common
    ${PROJECT_SOURCE_DIR}/common/coroutine
    ${PROJECT_SOURCE_DIR}/common/utils
    ${PROJECT_SOURCE_DIR}/app/protocol
    ${PROJECT_SOURCE_DIR}/app/application/common
    ${SERVICE_DIR}/base
    ${SERVICE_DIR}/maskword/common
    ${SERVICE_DIR}/maskword/logic
    ${SERVICE_DIR}/manager/common
    ${SERVICE_DIR}/manager/logic
    ${SERVICE_DIR}/login/common
    ${SERVICE_DIR}/login/logic
    ${SERVICE_DIR}/game/common
    ${SERVICE_DIR}/game/logic
    ${SERVICE_DIR}/client/common
    ${SERVICE_DIR}/client/logic
)

add_subdirectory(${PROJECT_SOURCE_DIR}/common)
add_subdirectory(${PROJECT_SOURCE_DIR}/app/protocol)

add_subdirectory(${SERVICE_DIR}/base)
add_subdirectory(${SERVICE_DIR}/maskword)
add_subdirectory(${SERVICE_DIR}/manager)
add_subdirectory(${SERVICE_DIR}/login)
add_subdirectory(${SERVICE_DIR}/game)

add_library(server_service
    $<TARGET_OBJECTS:manager_service_obj>
    $<TARGET_OBJECTS:login_service_obj>
    $<TARGET_OBJECTS:game_service_obj>
)

add_subdirectory(${SERVICE_DIR}/client)

add_subdirectory(${PROJECT_SOURCE_DIR}/app/application)
add_subdirectory(${PROJECT_SOURCE_DIR}/app/apptool)
