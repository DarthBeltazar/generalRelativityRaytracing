#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

#include "Structs.h"

#include <thread>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <string>


State f(const State &state, double rs) {
    return State(state.w, -state.u + 1.5 * rs * state.u * state.u);
}

State rk4Step(State y, double rs, double h) {
    State k1 = f(y, rs);
    State k2 = f(y + k1*(h/2), rs);
    State k3 = f(y + k2*(h/2), rs);
    State k4 = f(y + k3*h, rs);
    State y_next = y + (k1 + k2 + k2 + k3 + k3 + k4)*(h/6);
    return y_next;
}
std::vector<State> rk4(double rs, double h, const State &state0) {
    State y = state0;
    std::vector<State> output;
    output.reserve(1001);
    output.push_back(y);
    for (int i = 0; i < 1000; i++) {
        y = rk4Step(y, rs, h);
        output.push_back(y);
        if (y.u >= 1/rs || y.u <= 0) {
            break;
        }
    }
    return output;
}

struct Ray {
    Vec3 origin, dir;
    Ray(Vec3 origin, Vec3 dir): origin(origin), dir(dir) {}
    Ray(): origin(Vec3()), dir(Vec3()) {}
};

struct HitInfo {
    bool hit, discHit = false;
    double t;
    Vec3 pos;
    Vec3 dir;
    Vec3 normal;
};

Ray generateRay(const int px, const int py, double width, double height, double aspect, const Vec3 &origin, const double yaw, const double pitch) {
    Vec3 forward(
    cos(pitch) * sin(yaw),
      sin(pitch),
      -cos(pitch) * cos(yaw));
    Vec3 right = forward.cross(Vec3(0, 1, 0)).normalize();
    Vec3 up    = right.cross(forward);
    double u = (2.0 * (px + 0.5) / width - 1.0) * aspect;
    double v = 1.0 - 2.0 * (py + 0.5) / height;
    Vec3 direction = (right * u + up * v + forward).normalize();
    return Ray(origin, direction);
}

double square(double t) {return t*t;}

template <typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}


std::vector<Vec3> statesToVec(const std::vector<State> &states, const Vec3 &bhpos, double h, const Ray &ray, const Vec3 &r_vec) {
    Vec3 e_t = r_vec.cross(ray.dir).cross(r_vec).normalize();
    Vec3 e_r = r_vec.normalize();
    std::vector<Vec3> output(states.size());
    for (int i = 0; i < states.size(); i++) {
        output[i] = (e_r * cos(i*h) + e_t * sin(i*h)) * (1 / states[i].u);
    }
    return output;
}

HitInfo traceRay (const double h, const double rs, const int px, const int py, double width, double height, double aspect, const Vec3 &bhpos, const double yaw, const double pitch) {
    Ray ray = generateRay(px, py, width, height, aspect, Vec3(), yaw, pitch);
    Vec3 r_vec = ray.origin - bhpos;
    double r_cam = r_vec.length();
    double b0s = r_vec.cross(ray.dir).squaredLength();
    double bbs = (1 - rs / r_cam) / b0s;
    double u0 = 1 / r_cam;

    State state0(u0, -sqrt(rs*u0*u0*u0 - u0*u0 + bbs)*sgn(r_vec.dot(ray.dir)));
    auto states = rk4(rs, h, state0);
    HitInfo hi;
    hi.hit = states.back().u >= 1/rs;
    std::vector<Vec3> positions = statesToVec(states, bhpos, h, ray, r_vec);
    Vec3 prev = ray.origin - bhpos;
    double max = 50 * rs * rs;
    double min = 9 * rs * rs;
    for (int i = 0; i < positions.size(); i++) {
        Vec3 current = positions[i];
        if (current.y*prev.y < 0) {
            Vec3 delta = current - prev;
            hi.pos = prev - delta * prev.y * (1. / delta.y);
            double length = hi.pos.squaredLength();
            if (length < min || length > max) {
                prev = current;
                continue;
            }
            hi.discHit = true;
            break;
        }
        prev = current;
    }
    if (!hi.hit) {
        hi.dir = (positions.back() - positions[positions.size()-2]).normalize();
    }
    hi.t = states.size();
    return hi;
}

std::vector<HitInfo> traceRays(const double h, const double rs, const int width, const int height, const Vec3 &bhpos, const double yaw, const double pitch) {
    const unsigned int threadsNumber = std::thread::hardware_concurrency()-2;
    const int rowsPerThread = height / threadsNumber;

    std::vector<int> indexes;
    for (int i = 0; i < height; i += rowsPerThread) {
        indexes.push_back(i);
    }
    if (indexes.back() != height) indexes.push_back(height);


    double widthd = width;
    double heightd = height;
    double aspect = widthd / heightd;

    std::vector<HitInfo> output(width*height);
    std::vector<std::thread> threads;
    for (int i = 0; i < indexes.size() - 1;) {
        auto start = indexes[i++];
        auto end = indexes[i];

        threads.push_back(std::thread([yaw, pitch, start, end, h, rs, width, widthd, heightd, aspect, &bhpos, &output]()-> void {
            for (int y = start; y < end; y++) {
                for (int x = 0; x < width; x++) {
                    output[y*width+x] = traceRay(h, rs, x, y, widthd, heightd, aspect, bhpos, yaw, pitch);
                }
            }
        }));
    }
    for (auto& t : threads) {
        t.join();
    }
    return output;
}

double duration(std::chrono::time_point<std::chrono::steady_clock> t1, std::chrono::time_point<std::chrono::steady_clock> t2) {
    return static_cast<std::chrono::duration<double, std::milli>>(t2 - t1).count();
}

float* img;
int width, height;
const int channels = 4;

Vec3 backgroundColor(Vec3 dir) {
    const double rPI = 1/3.14159265359;
    int x = static_cast<int>((atan2(dir.z, dir.x)*rPI+1.5)*0.5 * width)%width;
    int y = static_cast<int>((asin(dir.y)*rPI+0.5) * height)%height;
    int index = (y*width + x)*channels;
    return Vec3(pow(static_cast<double>(img[index]),0.45), pow(static_cast<double>(img[index+1]), 0.45), pow(static_cast<double>(img[index+2]), 0.45));
}

Vec3 discColor(Vec3 pos) {
    double r = pos.length();
    double phi = atan2(pos.z,pos.x);
    double c = std::clamp(sin(r*6)*cos(phi*20)*100+0.2, 0., 1.);
    return Vec3(c*(r - 1.5)*0.3, c*0.2, c*(3.7 - r)*0.3);
}

void writeImage(std::vector<HitInfo> his, const int WIDTH, const int HEIGHT, const char *filename) {
    std::vector<unsigned char> data(WIDTH*HEIGHT*3);
    for (int i = 0; i < WIDTH*HEIGHT; i++) {
        auto hi = his[i];
        if (hi.t > 999) {
            data[i*3] = 0;
            data[i*3+1] = 255;
            data[i*3+2] = 0;
            continue;
        }
        if (hi.discHit) {
            Vec3 color = discColor(hi.pos);
            data[i*3] = static_cast<unsigned char>(color.x * 255);
            data[i*3+1] = static_cast<unsigned char>(color.y * 255);
            data[i*3+2] = static_cast<unsigned char>(color.z * 255);
            continue;
        }
        if (hi.hit) {
            data[i*3] = 0;
            data[i*3+1] = 0;
            data[i*3+2] = 0;
            continue;
        }
        Vec3 color = backgroundColor(hi.dir);

        data[i*3] = static_cast<unsigned char>(std::clamp(color.x*5, 0.0, 1.0) * 255);
        data[i*3+1] = static_cast<unsigned char>(std::clamp(color.y*5, 0.0, 1.0) * 255);
        data[i*3+2] = static_cast<unsigned char>(std::clamp(color.z*5, 0.0, 1.0) * 255);
    }
    stbi_write_png(filename, WIDTH, HEIGHT, 3, data.data(), WIDTH*3);
}

void loadBackground() {
    const char* err = nullptr;
    int ret = LoadEXR(&img, &width, &height, "../background.exr", &err);
    if (ret != TINYEXR_SUCCESS) {
        if (err) {
            std::cerr << "EXR load error: " << err << std::endl;
            FreeEXRErrorMessage(err);
        }
        throw std::runtime_error("Background load failed");
    }
}

void renderImage(const int WIDTH, const int HEIGHT, const char *filename, const double h, const double rs, const Vec3 &bhpos, const double yaw, const double pitch) {
    auto his = traceRays(h, rs, WIDTH, HEIGHT, bhpos, yaw, pitch);
    writeImage(his, WIDTH, HEIGHT, filename);
}

int main() {
    const double h = 0.01;
    const int WIDTH = 1920;
    const int HEIGHT = 1080;
    auto t1 = std::chrono::high_resolution_clock::now();
    loadBackground();
    auto t2 = std::chrono::high_resolution_clock::now();
    auto his = traceRays(h, 0.5, WIDTH, HEIGHT, Vec3(0, -1, -2.5), 0, -0.4);
    auto t3 = std::chrono::high_resolution_clock::now();
    writeImage(his, WIDTH, HEIGHT, "output.png");
    auto t4 = std::chrono::high_resolution_clock::now();
    std::cout << duration(t1, t2) << " " << duration(t2, t3) << " " << duration(t3, t4);
    // const double r = 6;
    // for (int i = 0; i < 180; i ++) {
    //     double phi = static_cast<double>(i-89) / 180 * 3.141593;
    //     bool invert = 0;
    //     auto his = traceRays(h, 0.5, WIDTH, HEIGHT, Vec3(0, -sin(phi) * r, -cos(phi) * r), 3.141593 * invert, -phi);
    //     std::string filename = "../seq/output_" + std::to_string(i) +".png";
    //     writeImage(his, WIDTH, HEIGHT, filename.c_str());
    //     std::cout << i << std::endl << std::endl;
    //
    // }

    free(img);
    return 0;
}
