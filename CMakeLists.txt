cmake_minimum_required(VERSION 3.31)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
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

FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.14
        GIT_SHALLOW TRUE
)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL3)

add_executable(grRaytracing
        src/main.cpp
        src/core/Constants.h
        src/core/Vec3.cpp
        src/core/Vec3.h
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
        src/render/Camera.cpp
        src/render/Camera.h
)
target_include_directories(grRaytracing PRIVATE
        src
        ${stb_SOURCE_DIR}
        ${tinyexr_SOURCE_DIR}
        ${tinyexr_SOURCE_DIR}/deps/miniz
)

add_executable(trace_pixel_step
        src/tools/PixelStepTrace.cpp
        src/core/Vec3.cpp
        src/core/Vec3.h
        src/physics/physics.cpp
        src/physics/physics.h
        src/render/Camera.cpp
        src/render/Camera.h
)
target_include_directories(trace_pixel_step PRIVATE src)

add_executable(viewer
        src/app/Viewer.cpp
        src/core/Constants.h
        src/core/Vec3.cpp
        src/core/Vec3.h
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
        src/render/Camera.cpp
        src/render/Camera.h
)
target_include_directories(viewer PRIVATE
        src
        ${stb_SOURCE_DIR}
        ${tinyexr_SOURCE_DIR}
        ${tinyexr_SOURCE_DIR}/deps/miniz
)
target_link_libraries(viewer PRIVATE SDL3::SDL3-static)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(grRaytracing PRIVATE -fno-omit-frame-pointer -static -static-libgcc -static-libstdc++)
endif()

target_compile_definitions(grRaytracing PRIVATE GR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_compile_definitions(viewer PRIVATE GR_SOURCE_DIR="${CMAKE_SOURCE_DIR}")