#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "Renderer.h"
#include "AccretionDisc.h"
#include "Camera.h"
#include "physics/physics.h"

#include <thread>
#include <algorithm>
#include <cmath>
#include <limits>

#include "core/Constants.h"

namespace {
    struct Tile {
        int x0, y0, x1, y1; //[x0, x1), [y0, y1)
        Tile (int x0, int x1, int y0, int y1): x0(x0), x1(x1), y0(y0), y1(y1) {};
    };
    unsigned int workerThreadCount() {
        return static_cast<unsigned int>(std::max(1, static_cast<int>(std::thread::hardware_concurrency()) - 2));
    }
}

std::vector<HitInfo> traceRays(const double h, const double rs, const int width, const int height, const Vec3 &bhpos,
                               const CameraBasis &basis) {
    const unsigned int threadsNumber = workerThreadCount();

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
                            Ray ray = generateRay(x, y, widthd, heightd, aspect, Vec3(), basis);
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

struct Vec2 {
    double x, y;
    Vec2(double x, double y): x(x), y(y) {}
    Vec2(): x(0), y(0) {}
};

inline double getFrac(double num) {
    return num - std::floor(num);
}

inline double footprintHalfSize(const std::vector<HitInfo> &his, const std::vector<Vec2> &textels, const int i,
                                const int stride, double Vec2::*axis, const bool wrap) {
    auto delta = [&](const int a, const int b) {
        double d = std::abs(textels[a].*axis - textels[b].*axis);
        if (wrap && d > 0.5) d = 1 - d;
        return d;
    };
    const bool prevOk = !his[i - stride].hit;
    const bool nextOk = !his[i + stride].hit;
    if (prevOk && nextOk) return delta(i + stride, i - stride) * 0.25;
    if (prevOk) return delta(i, i - stride) * 0.5;
    if (nextOk) return delta(i + stride, i) * 0.5;
    return 0;
}

std::vector<unsigned char> shade(const std::vector<HitInfo> &his, const int WIDTH, const int HEIGHT, double time,
                const Background &background)  {
    std::vector<unsigned char> data(WIDTH * HEIGHT * 3);
    const unsigned int threadsNumber = workerThreadCount();

    constexpr int tileSize = 64;
    std::vector<Tile> tiles;
    for (int y = 0; y < HEIGHT;) {
        const int y1 = std::min(y + tileSize, HEIGHT);
        for (int x = 0; x < WIDTH;) {
            const int x1 = std::min(x + tileSize, WIDTH);
            tiles.push_back(Tile(x, x1, y, y1));
            x = x1;
        }
        y = y1;
    }
    std::vector<Vec2> textels(his.size());
    std::vector<std::thread> threads(threadsNumber);
    std::atomic<int> nextIndex = 0;
    for (int i = 0; i < threadsNumber; i++) {
        threads[i] = std::thread(
            [&]()-> void {
                int index;
                for (int j = 0; j < tiles.size(); j++) {
                    index = nextIndex++;
                    if (index >= tiles.size()) break;
                    Tile tile = tiles[index];
                    for (int y = tile.y0; y < tile.y1; y++) {
                        for (int x = tile.x0; x < tile.x1; x++) {
                            int i = y*WIDTH+x;
                            const auto &hi = his[i];
                            if (hi.hit) {textels[i] = Vec2(0.5, 0.5); continue;}

                            const Vec3 dir = hi.dir;
                            const double xt = getFrac((atan2(dir.z, dir.x) * RECIP_PI + 1.5) * 0.5);
                            const double yt = getFrac(asin(dir.y) * RECIP_PI + 0.5);
                            textels[i] = Vec2(xt, yt);
                        }
                    }
                }
            });
    }
    for (auto &t: threads) {
        t.join();
    }
    nextIndex = 0;
    for (int i = 0; i < threadsNumber; i++) {
        threads[i] = std::thread(
            [&]()-> void {
                int index;
                for (int j = 0; j < tiles.size(); j++) {
                    index = nextIndex++;
                    if (index >= tiles.size()) break;
                    Tile tile = tiles[index];
                    for (int y = tile.y0; y < tile.y1; y++) {
                        for (int x = tile.x0; x < tile.x1; x++) {
                            int i = y*WIDTH+x;
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
                            Vec3 backgroundColor(0, 0, 0);
                            if (!hi.hit) {
                                if (x == 0 || y == 0 || x == WIDTH-1 || y == HEIGHT-1) backgroundColor = background.sample(hi.dir);
                                else {
                                    const double dx = footprintHalfSize(his, textels, i, 1, &Vec2::x, true);
                                    const double dy = footprintHalfSize(his, textels, i, WIDTH, &Vec2::y, false);
                                    double x0 = textels[i].x - dx;
                                    double x1 = textels[i].x + dx;
                                    double y0 = textels[i].y - dy;
                                    double y1 = textels[i].y + dy;
                                    backgroundColor = background.sampleAA(x0, y0, x1, y1);
                                }
                            }

                            color = (color + backgroundColor * 35).custom([](double x) -> double { return pow(x, 0.45); });

                            data[i * 3] = static_cast<unsigned char>(std::clamp(color.x, 0.0, 1.0) * 255);
                            data[i * 3 + 1] = static_cast<unsigned char>(std::clamp(color.y, 0.0, 1.0) * 255);
                            data[i * 3 + 2] = static_cast<unsigned char>(std::clamp(color.z, 0.0, 1.0) * 255);
                        }
                    }
                }
            });
    }
    for (auto &t: threads) {
        t.join();
    }
    return data;
}

template<typename F>
bool pixelDerivative(const F &value, const int i, const int stride, const bool prevOk, const bool nextOk, Vec3 &out) {
    if (prevOk && nextOk) out = (value(i + stride) - value(i - stride)) * 0.5;
    else if (nextOk) out = value(i + stride) - value(i);
    else if (prevOk) out = value(i) - value(i - stride);
    else return false;
    return true;
}

std::vector<unsigned char> shadeMagnification(const std::vector<HitInfo> &his, const int WIDTH, const int HEIGHT,
                                              const CameraBasis &basis) {
    const Vec3 white(1, 1, 1);
    const Vec3 demagnified(0.1, 0.3, 1.0);
    const Vec3 magnified(1.0, 0.25, 0.05);

    const double widthd = WIDTH;
    const double heightd = HEIGHT;
    const double aspect = widthd / heightd;
    auto cameraDir = [&](const int idx) {
        return generateRay(idx % WIDTH, idx / WIDTH, widthd, heightd, aspect, Vec3(), basis).dir;
    };
    auto skyDir = [&](const int idx) { return his[idx].dir; };

    std::vector<double> logMu(WIDTH * HEIGHT, std::numeric_limits<double>::quiet_NaN());
    std::vector<double> absLogMu;
    absLogMu.reserve(WIDTH * HEIGHT);
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            const int i = y * WIDTH + x;
            if (his[i].hit) continue;

            Vec3 skyDx, skyDy;
            if (!pixelDerivative(skyDir, i, 1, x > 0 && !his[i - 1].hit, x < WIDTH - 1 && !his[i + 1].hit, skyDx) ||
                !pixelDerivative(skyDir, i, WIDTH, y > 0 && !his[i - WIDTH].hit, y < HEIGHT - 1 && !his[i + WIDTH].hit, skyDy)) {
                continue;
            }
            Vec3 camDx, camDy;
            pixelDerivative(cameraDir, i, 1, x > 0, x < WIDTH - 1, camDx);
            pixelDerivative(cameraDir, i, WIDTH, y > 0, y < HEIGHT - 1, camDy);

            const double skyArea = skyDx.cross(skyDy).length();
            const double camArea = camDx.cross(camDy).length();
            logMu[i] = std::log2(camArea / skyArea);

            if (std::isfinite(logMu[i])) absLogMu.push_back(std::abs(logMu[i]));
        }
    }

    constexpr double percentile = 0.99;
    double scaleLogMu = 0;
    if (!absLogMu.empty()) {
        const auto nth = absLogMu.begin() + static_cast<std::ptrdiff_t>(percentile * (absLogMu.size() - 1));
        std::nth_element(absLogMu.begin(), nth, absLogMu.end());
        scaleLogMu = *nth;
    }
    const double c = scaleLogMu > 0 ? 1 / scaleLogMu : 0;
    std::vector<unsigned char> data(WIDTH * HEIGHT * 3, 0);
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        if (std::isnan(logMu[i])) continue;
        const double t = std::clamp(logMu[i] * c, -1.0, 1.0);
        const Vec3 color = t < 0 ? white + (demagnified - white) * -t : white + (magnified - white) * t;

        data[i * 3] = static_cast<unsigned char>(std::clamp(color.x, 0.0, 1.0) * 255);
        data[i * 3 + 1] = static_cast<unsigned char>(std::clamp(color.y, 0.0, 1.0) * 255);
        data[i * 3 + 2] = static_cast<unsigned char>(std::clamp(color.z, 0.0, 1.0) * 255);
    }
    return data;
}

void writeImage(const std::vector<HitInfo> &his, const int WIDTH, const int HEIGHT, const char *filename, double time,
                const Background &background) {
    const auto data = shade(his, WIDTH, HEIGHT, time, background);
    stbi_write_png(filename, WIDTH, HEIGHT, 3, data.data(), WIDTH * 3);
}

void renderImage(const int WIDTH, const int HEIGHT, const char *filename, const double h, const double rs,
                 const Vec3 &bhpos, const double yaw, const double pitch, double time, const Background &background) {
    auto his = traceRays(h, rs, WIDTH, HEIGHT, bhpos, computeCameraBasis(yaw, pitch));
    writeImage(his, WIDTH, HEIGHT, filename, time, background);
}