Project(client_pressure_service)

include_directories(
    common
    logic
)

file(GLOB_RECURSE FileSources *.cpp *.c *.cc)
file(GLOB_RECURSE FileHeaders *.h *.hpp)

add_library(client_pressure_service ${FileHeaders} ${FileSources})

