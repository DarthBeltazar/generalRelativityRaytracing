#include "core/Vec3.h"
#include "physics/physics.h"
#include "render/Camera.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

namespace {
    constexpr int WIDTH = 1920;
    constexpr int HEIGHT = 1080;
    constexpr double H0 = 0.01;
    constexpr double RS = 0.5;
    const Vec3 BHPOS(0, -0.4, -5);
    constexpr double YAW = 0;
    constexpr double PITCH = -0.04;

    std::optional<int> readEnvInt(const char *name) {
        const char *env = std::getenv(name);
        if (!env || *env == '\0') {
            return std::nullopt;
        }
        char *end = nullptr;
        const long value = std::strtol(env, &end, 10);
        if (end == env || *end != '\0') {
            return std::nullopt;
        }
        return static_cast<int>(value);
    }
}

int main() {
    const auto px = readEnvInt("GR_TRACE_PIXEL_X");
    const auto py = readEnvInt("GR_TRACE_PIXEL_Y");
    if (!px || !py) {
        std::cerr << "GR_TRACE_PIXEL_X and GR_TRACE_PIXEL_Y must both be set to integer pixel indices."
                << std::endl;
        return 1;
    }
    if (*px < 0 || *px >= WIDTH || *py < 0 || *py >= HEIGHT) {
        std::cerr << "Pixel (" << *px << ", " << *py << ") is out of bounds for a " << WIDTH << "x" << HEIGHT
                << " frame." << std::endl;
        return 1;
    }

    const char *csvEnv = std::getenv("GR_TRACE_CSV");
    const std::string csvPath = (csvEnv && *csvEnv != '\0') ? csvEnv : "step_trace.csv";

    std::ofstream csv(csvPath);
    if (!csv) {
        std::cerr << "Could not open '" << csvPath << "' for writing." << std::endl;
        return 1;
    }
    csv << "step,distance_rs,h\n";

    const CameraBasis basis = computeCameraBasis(YAW, PITCH);
    const double aspect = static_cast<double>(WIDTH) / HEIGHT;
    const Ray ray = generateRay(*px, *py, WIDTH, HEIGHT, aspect, Vec3(), basis);

    const HitInfo hit = traceRay(H0, RS, BHPOS, ray, [&](int step, double distance, double h) {
        csv << step << "," << (distance / RS) << "," << h << "\n";
    });

    std::cout << "Pixel (" << *px << ", " << *py << "): " << hit.t << " steps, "
            << (hit.hit ? "hit event horizon" : "escaped") << ". Wrote " << csvPath << std::endl;
    return 0;
}