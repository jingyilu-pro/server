Project(application)

include_directories(
    common
    logic
    logic/aoi
    logic/maskword
)

file(GLOB_RECURSE FileSources  *.cpp *.c *.cc)
file(GLOB_RECURSE FileHeaders *.h *.hpp)

add_executable(application ${FileHeaders} ${FileSources})
target_link_libraries(application ${COMMON_LIB} ${THIRD_LIB_DIR} ${CMAKE_DL_LIBS})
target_compile_features(application PRIVATE cxx_std_20)

install(TARGETS application DESTINATION ${CMAKE_INSTALL_PREFIX})

