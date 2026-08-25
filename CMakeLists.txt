cmake_minimum_required(VERSION 3.31)
project(grRaytracing)

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

add_executable(grRaytracing
        src/main.cpp
        src/core/Constants.h
        src/core/Structs.cpp
        src/core/Structs.h
        ${tinyexr_SOURCE_DIR}/deps/miniz/miniz.c
        src/physics/physics.cpp
        src/physics/physics.h
        src/render/PerlinNoise.cpp
        src/render/PerlinNoise.h
        src/render/Background.cpp
        src/render/Background.h
        src/render/AccretionDisc.cpp
        src/render/AccretionDisc.h
        src/render/Renderer.cpp
        src/render/Renderer.h
)
target_include_directories(grRaytracing PRIVATE
        src
        ${stb_SOURCE_DIR}
        ${tinyexr_SOURCE_DIR}
        ${tinyexr_SOURCE_DIR}/deps/miniz
)

# Keep frame pointers so profilers (perf, gprof) can unwind the call stack in release builds.
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(grRaytracing PRIVATE -fno-omit-frame-pointer)
endif()

# Lets the app find its bundled assets (background.exr, seq/) by source location
# instead of by a relative "../" path, which breaks whenever the build directory
# isn't a direct sibling of the source tree (e.g. a WSL-native build dir profiling
# a project that lives on the Windows-mounted filesystem).
target_compile_definitions(grRaytracing PRIVATE GR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")