include_directories(
        protocol
)

file(GLOB_RECURSE FileSources *.cpp *.c *.cc)
file(GLOB_RECURSE FileHeaders *.h *.hpp)

add_library(protocol ${FileHeaders} ${FileSources})
