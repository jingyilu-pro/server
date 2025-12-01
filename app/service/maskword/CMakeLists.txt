Project(maskword_service)

include_directories(
    common
    logic
)


file(GLOB_RECURSE FileSources  *.cpp *.c *.cc)
file(GLOB_RECURSE FileHeaders *.h *.hpp)

add_library(maskword_service ${FileHeaders} ${FileSources})
