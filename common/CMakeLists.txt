include_directories(${PROJECT_SOURCE_DIR})
file(GLOB_RECURSE FileSources  *.cpp *.c *.cc)
file(GLOB_RECURSE FileHeaders *.h *.hpp)

add_library(common ${FileHeaders} ${FileSources})
