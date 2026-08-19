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
#include <numeric>
#include <random>
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
    std::vector<Vec3> pos;
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
            hi.pos.push_back(prev - delta * prev.y * (1. / delta.y));
            hi.discHit = true;
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
    return Vec3(img[index], img[index+1], img[index+2]);
}

class PerlinNoise {
private:
    std::vector<int> p;

    double fade(double t) {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }
    double lerp(double t, double a, double b) {
        return a + t * (b - a);
    }

    double grad(int hash, double x, double y, double z) {
        int h = hash & 15;
        double u = h < 8 ? x : y;
        double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

public:
    PerlinNoise() {
        p = {
            151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,8,99,37,240,21,10,23,
            190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,
            20,125,136,171,168, 68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,
            230,220,105,92,41,55,46,245,40,244,102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,208, 89,
            18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,250,124,123,5,202,
            38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,
            2,44,154,163, 70,221,153,101,155,167, 43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185,
            112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,107,
            49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,138,236,205,93,222,114,
            67,29,24,72,243,141,128,195,78,212,88,119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
            129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,228,251,34,242,193,238,210,
            144,12,191,179,162,241,81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,184,84,204,176,
            115,121,50,45,127,4,150,254,138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,212,88,119,248,
            152,2,44,154,163,70,221,153,101,155,167,43,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,
            185,112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
            107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,222,114,
            67,29,24,72,243,141,128,195,78
        };
        p.insert(p.end(), p.begin(), p.end());
    }

    PerlinNoise(unsigned int seed) {
        p.resize(256);
        std::iota(p.begin(), p.end(), 0);
        std::default_random_engine engine(seed);
        std::shuffle(p.begin(), p.end(), engine);
        p.insert(p.end(), p.begin(), p.end());
    }

    double noise(double x, double y, double z) {
        int X = static_cast<int>(std::floor(x)) & 255;
        int Y = static_cast<int>(std::floor(y)) & 255;
        int Z = static_cast<int>(std::floor(z)) & 255;

        x -= std::floor(x);
        y -= std::floor(y);
        z -= std::floor(z);

        double u = fade(x);
        double v = fade(y);
        double w = fade(z);

        int A  = p[X] + Y;
        int AA = p[A] + Z;
        int AB = p[A + 1] + Z;
        int B  = p[X + 1] + Y;
        int BA = p[B] + Z;
        int BB = p[B + 1] + Z;

        return lerp(w, lerp(v, lerp(u, grad(p[AA],     x,     y,     z),
                                       grad(p[BA],     x - 1, y,     z)),
                               lerp(u, grad(p[AB],     x,     y - 1, z),
                                       grad(p[BB],     x - 1, y - 1, z))),
                       lerp(v, lerp(u, grad(p[AA + 1], x,     y,     z - 1),
                                       grad(p[BA + 1], x - 1, y,     z - 1)),
                               lerp(u, grad(p[AB + 1], x,     y - 1, z - 1),
                                       grad(p[BB + 1], x - 1, y - 1, z - 1))));
    }
};

static PerlinNoise perlinGenerator(1337);

float fbm_noise(float x, float z, float time) {
    float total = 0.0f;
    float amplitude = 2.0f;
    float frequency = 2.0f;
    float maxValue = 0.0f;

    const int OCTAVES = 4;

    for (int i = 0; i < OCTAVES; ++i) {
        double rawNoise = perlinGenerator.noise(x * frequency, time * frequency, z * frequency);

        float normalizedNoise = (static_cast<float>(rawNoise) + 1.0f) * 0.5f;

        total += normalizedNoise * amplitude;
        maxValue += amplitude;

        amplitude *= 0.7f;
        frequency *= 3.0f;
    }

    // Возвращаем строго в диапазоне [0.0, 1.0]
    return total / maxValue;
}

Vec3 discColor(Vec3 pos, double time) {
    double r = pos.length();
    double angle = atan2(pos.z,pos.x);

    const float R_IN = 1.5f;
    const float R_OUT = 4.5;

    if (r < R_IN || r > R_OUT) return {0.0f, 0.0f, 0.0f};


    float angularVelocity = 1.5f / (r * std::sqrt(r));
    float twistedAngle = angle - time * angularVelocity;

    float scale =.5f;
    float noiseX = r * std::cos(twistedAngle) * scale;
    float noiseZ = r * std::sin(twistedAngle) * scale;

    float plasmaDensity = fbm_noise(noiseX, noiseZ, time * 0.1f);

    float distanceFactor = (R_OUT - r) / (R_OUT - R_IN);
    float baseTemperature = std::pow(distanceFactor, 2.0f) * (R_IN / r);

    float currentTemperature = baseTemperature * (0.2f + 0.8f * plasmaDensity);


    Vec3 finalColor = {0.0f, 0.0f, 0.0f};

    if (currentTemperature > 0.05f) {
        finalColor.x = std::clamp(currentTemperature * 3.0f, 0.0f, 1.0f);
        finalColor.y = std::clamp((currentTemperature - 0.2f) * 2.5f, 0.0f, 1.0f);
        finalColor.z = std::clamp((currentTemperature - 0.6f) * 4.0f, 0.0f, 1.0f);
    }
    return finalColor;
}

void writeImage(std::vector<HitInfo> his, const int WIDTH, const int HEIGHT, const char *filename, double time) {
    std::vector<unsigned char> data(WIDTH*HEIGHT*3);
    for (int i = 0; i < WIDTH*HEIGHT; i++) {
        auto hi = his[i];
        Vec3 color(0, 0, 0);
        // if (hi.t > 999) {
        //     data[i*3] = 0;
        //     data[i*3+1] = 255;
        //     data[i*3+2] = 0;
        //     continue;
        // }
        if (hi.discHit) {
            for (const auto &p : hi.pos) {
                color = color + discColor(p, time);
            }
        }
        if (hi.hit) {
            data[i*3] = static_cast<unsigned char>(std::clamp(color.x, 0.0, 1.0) * 255);
            data[i*3+1] = static_cast<unsigned char>(std::clamp(color.y, 0.0, 1.0) * 255);
            data[i*3+2] = static_cast<unsigned char>(std::clamp(color.z, 0.0, 1.0) * 255);
            continue;
        }
        color = (color + backgroundColor(hi.dir)*35).custom([](double x) -> double {return pow(x, 0.45);});

        data[i*3] = static_cast<unsigned char>(std::clamp(color.x, 0.0, 1.0) * 255);
        data[i*3+1] = static_cast<unsigned char>(std::clamp(color.y, 0.0, 1.0) * 255);
        data[i*3+2] = static_cast<unsigned char>(std::clamp(color.z, 0.0, 1.0) * 255);
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

void renderImage(const int WIDTH, const int HEIGHT, const char *filename, const double h, const double rs, const Vec3 &bhpos, const double yaw, const double pitch, double time) {
    auto his = traceRays(h, rs, WIDTH, HEIGHT, bhpos, yaw, pitch);
    writeImage(his, WIDTH, HEIGHT, filename, time);
}

int main() {
    const double h = 0.01;
    const int WIDTH = 1920;
    const int HEIGHT = 1080;
    auto t1 = std::chrono::high_resolution_clock::now();
    loadBackground();
    auto t2 = std::chrono::high_resolution_clock::now();
    auto his = traceRays(h, 0.5, WIDTH, HEIGHT, Vec3(0, -0.4, -5), 0, -0.04);
    auto t3 = std::chrono::high_resolution_clock::now();
    writeImage(his, WIDTH, HEIGHT, "output.png", 0);
    auto t4 = std::chrono::high_resolution_clock::now();
    std::cout << duration(t1, t2) << " " << duration(t2, t3) << " " << duration(t3, t4);
    const double r = 6;
    for (int i = 0; i < 360; i ++) {
        double phi = static_cast<double>(i-89) / 180 * 3.141593;
        bool invert = 0;
        auto his = traceRays(h, 0.5, WIDTH, HEIGHT, Vec3(-4*sin(-phi), -0.2, -4*cos(-phi)), phi, -0.05);
        std::string filename = "../seq/output_" + std::to_string(i) +".png";
        writeImage(his, WIDTH, HEIGHT, filename.c_str(), static_cast<double>(i)*0.05);
        std::cout << i << std::endl << std::endl;
    }

    free(img);
    return 0;
}
