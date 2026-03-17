Project(application)

include_directories(
        common
        logic
)

file(GLOB_RECURSE FileSources *.cpp *.c *.cc)
file(GLOB_RECURSE FileHeaders *.h *.hpp)

MESSAGE(STATUS " SERVICE_LIB = " ${SERVICE_LIB})

add_executable(application ${FileHeaders} ${FileSources})
target_link_libraries(application PRIVATE
        # service
        ${SERVICE_LIB}
        # servicebase
        ${SERVICE_BASE_DIR}
        # server_service is referenced from servicebase after the MUD repository wiring
        server_service
        # last
        ${COMMON_LIB}
        ${PROTOCOL_LIB}
        ${DEPENDENCIES_LIBS}
        ${MAIN_LIB_DIR}
        ${CMAKE_DL_LIBS}
)

if(TARGET application)
    add_dependencies(application
        libevent_external_project
    )
endif()


target_compile_features(application PRIVATE cxx_std_20)

install(TARGETS application DESTINATION ${CMAKE_INSTALL_PREFIX})
