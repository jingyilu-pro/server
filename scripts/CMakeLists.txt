Project(mud_smoke)

add_executable(mud_smoke mud_smoke.cpp)

target_link_libraries(mud_smoke PRIVATE
    ${COMMON_LIB}
    ${PROTOCOL_LIB}
    ${DEPENDENCIES_LIBS}
    ${MAIN_LIB_DIR}
    ${CMAKE_DL_LIBS}
)

target_compile_features(mud_smoke PRIVATE cxx_std_20)

install(TARGETS mud_smoke DESTINATION ${CMAKE_INSTALL_PREFIX})
