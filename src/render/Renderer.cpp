#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Renderer.h"
#include "AccretionDisc.h"
#include "physics/physics.h"

#include <thread>
#include <algorithm>
#include <cmath>

namespace {
    // forward/right/up depend only on yaw/pitch, which are constant for the whole
    // frame - computed once by the caller instead of on everyone of the 2M+ pixels
    Ray generateRay(const int px, const int py, double width, double height, double aspect, const Vec3 &origin,
                    const Vec3 &forward, const Vec3 &right, const Vec3 &up) {
        double u = (2.0 * (px + 0.5) / width - 1.0) * aspect;
        double v = 1.0 - 2.0 * (py + 0.5) / height;
        Vec3 direction = (right * u + up * v + forward).normalize();
        return Ray(origin, direction);
    }

    struct Tile {
        int x0, y0, x1, y1; //[x0, x1), [y0, y1)
        Tile (int x0, int x1, int y0, int y1): x0(x0), x1(x1), y0(y0), y1(y1) {};
    };
}

std::vector<HitInfo> traceRays(const double h, const double rs, const int width, const int height, const Vec3 &bhpos,
                               const double yaw, const double pitch) {
    const unsigned int threadsNumber = std::max(1u, std::thread::hardware_concurrency() - 2);

    constexpr int tileSize = 32;
    std::vector<Tile> tiles;
    for (int y = 0; y < height;) {
        const int y1 = std::min(y + tileSize, height);
        for (int x = 0; x < width;) {
            const int x1 = std::min(x + tileSize, width);
            tiles.push_back(Tile(x, x1, y, y1));
            x = x1;
        }
        y = y1;
    }
    Vec3 forward(
        cos(pitch) * sin(yaw),
        sin(pitch),
        -cos(pitch) * cos(yaw));
    Vec3 right = forward.cross(Vec3(0, 1, 0)).normalize();
    Vec3 up = right.cross(forward);

    std::vector<HitInfo> output(width * height);

    double widthd = width;
    double heightd = height;
    double aspect = widthd / heightd;
    std::vector<std::thread> threads(threadsNumber);
    std::atomic<int> nextIndex = 0;
    for (int i = 0; i < threadsNumber; i++) {
        threads[i] = (std::thread(
            [&]()-> void {
                int index;
                for (int i = 0; i < tiles.size(); i++) {
                    index = nextIndex++;
                    if (index >= tiles.size()) break;
                    Tile tile = tiles[index];
                    for (int y = tile.y0; y < tile.y1; y++) {
                        for (int x = tile.x0; x < tile.x1; x++) {
                            Ray ray = generateRay(x, y, widthd, heightd, aspect, Vec3(), forward, right, up);
                            output[y * width + x] = traceRay(h, rs, bhpos, ray);
                        }
                    }
                }
            }));
    }
    for (auto &t: threads) {
        t.join();
    }
    return output;
}

double duration(std::chrono::high_resolution_clock::time_point t1, std::chrono::high_resolution_clock::time_point t2) {
    return static_cast<std::chrono::duration<double, std::milli>>(t2 - t1).count();
}

void writeImage(const std::vector<HitInfo> &his, const int WIDTH, const int HEIGHT, const char *filename, double time,
                const Background &background) {
    std::vector<unsigned char> data(WIDTH * HEIGHT * 3);
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        const auto &hi = his[i];
        Vec3 color(0, 0, 0);
        // if (hi.t > 50) {
        //     data[i*3] = 0;
        //     data[i*3+1] = 255;
        //     data[i*3+2] = 0;
        //     continue;
        // }
        if (hi.discHit) {
            for (const auto &p: hi.pos) {
                color = color + discColor(p, time);
            }
        }
        if (hi.hit) {
            data[i * 3] = static_cast<unsigned char>(std::clamp(color.x, 0.0, 1.0) * 255);
            data[i * 3 + 1] = static_cast<unsigned char>(std::clamp(color.y, 0.0, 1.0) * 255);
            data[i * 3 + 2] = static_cast<unsigned char>(std::clamp(color.z, 0.0, 1.0) * 255);
            continue;
        }
        color = (color + background.sample(hi.dir) * 35).custom([](double x) -> double { return pow(x, 0.45); });

        data[i * 3] = static_cast<unsigned char>(std::clamp(color.x, 0.0, 1.0) * 255);
        data[i * 3 + 1] = static_cast<unsigned char>(std::clamp(color.y, 0.0, 1.0) * 255);
        data[i * 3 + 2] = static_cast<unsigned char>(std::clamp(color.z, 0.0, 1.0) * 255);
    }
    stbi_write_png(filename, WIDTH, HEIGHT, 3, data.data(), WIDTH * 3);
}

void renderImage(const int WIDTH, const int HEIGHT, const char *filename, const double h, const double rs,
                 const Vec3 &bhpos, const double yaw, const double pitch, double time, const Background &background) {
    auto his = traceRays(h, rs, WIDTH, HEIGHT, bhpos, yaw, pitch);
    writeImage(his, WIDTH, HEIGHT, filename, time, background);
}
