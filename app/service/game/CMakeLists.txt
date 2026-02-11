Project(game_service_obj)

include_directories(
    common
    logic
)

file(GLOB_RECURSE FileSources *.cpp *.c *.cc)
file(GLOB_RECURSE FileHeaders *.h *.hpp)

add_library(game_service_obj OBJECT ${FileHeaders} ${FileSources})

