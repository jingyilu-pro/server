Project(server_service)

include_directories(
    common
)

file(GLOB_RECURSE FileSources *.cpp *.c *.cc)
file(GLOB_RECURSE FileHeaders *.h *.hpp)

add_library(server_service ${FileHeaders} ${FileSources})

