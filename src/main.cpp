#include "core/Constants.h"
#include "core/Vec3.h"
#include "render/Background.h"
#include "render/Renderer.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

// Defined by CMake as the absolute path to the source tree, so bundled assets
// (background.exr, seq/) resolve regardless of where the build directory lives.
#ifndef GR_SOURCE_DIR
#define GR_SOURCE_DIR "."
#endif

int main() {
    const double h = 0.01;
    const int WIDTH = 1920;
    const int HEIGHT = 1080;
    // Full run renders 360 frames; profiling runs only need a handful of representative
    // ones, so this lets a profiler script cut the sequence short via an env var.
    int frameCount = 360;
    if (const char *env = std::getenv("GR_PROFILE_FRAMES")) {
        frameCount = std::max(0, static_cast<int>(std::strtol(env, nullptr, 10)));
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    Background background;
    background.load(GR_SOURCE_DIR "/background.exr");
    auto t2 = std::chrono::high_resolution_clock::now();
    auto his = traceRays(h, 0.5, WIDTH, HEIGHT, Vec3(0, -0.4, -5), 0, -0.04);
    auto t3 = std::chrono::high_resolution_clock::now();
    writeImage(his, WIDTH, HEIGHT, "output.png", 0, background);
    auto t4 = std::chrono::high_resolution_clock::now();
    std::cout << duration(t1, t2) << " " << duration(t2, t3) << " " << duration(t3, t4) << std::endl << std::endl;
    for (int i = 0; i < frameCount; i++) {
        double phi = static_cast<double>(i - 89) / 180 * PI;
        auto frameHis = traceRays(h, 0.5, WIDTH, HEIGHT, Vec3(-4 * sin(-phi), -0.2, -4 * cos(-phi)), phi, -0.05);
        std::string filename = std::string(GR_SOURCE_DIR) + "/seq/output_" + std::to_string(i) + ".png";
        writeImage(frameHis, WIDTH, HEIGHT, filename.c_str(), static_cast<double>(i) * 0.05, background);
        std::cout << i << std::endl;
    }

    return 0;
}
