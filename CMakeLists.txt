cmake_minimum_required(VERSION 3.31)
project(cppTraining)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

FetchContent_Declare(
        tinyexr
        GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
        GIT_TAG release
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(tinyexr)

add_executable(cppTraining main.cpp
        Structs.cpp
        Structs.h
        ${tinyexr_SOURCE_DIR}/deps/miniz/miniz.c
)
target_include_directories(cppTraining PRIVATE
        ${stb_SOURCE_DIR}
        ${tinyexr_SOURCE_DIR}
        ${tinyexr_SOURCE_DIR}/deps/miniz
)