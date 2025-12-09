Project(apptool)

include_directories(
        common
        logic
)

file(GLOB_RECURSE FileSources *.cpp *.c *.cc)
file(GLOB_RECURSE FileHeaders *.h *.hpp)

add_executable(apptool ${FileHeaders} ${FileSources})
target_link_libraries(apptool PRIVATE
        # service
        ${SERVICE_LIB}
        # servicebase
        ${SERVICE_BASE_DIR}
        # last
        ${COMMON_LIB}
        ${PROTOCOL_LIB}
        ${DEPENDENCIES_LIBS}
        ${MAIN_LIB_DIR}
        ${CMAKE_DL_LIBS}
)


target_compile_features(apptool PRIVATE cxx_std_20)

install(TARGETS apptool DESTINATION ${CMAKE_INSTALL_PREFIX})

