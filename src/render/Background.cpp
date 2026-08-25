#include "Background.h"

#define TINYEXR_IMPLEMENTATION
#include "tinyexr.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

Background::~Background() {
    free(img);
}

void Background::load(const char *path) {
    const char *err = nullptr;
    int ret = LoadEXR(&img, &width, &height, path, &err);
    if (ret != TINYEXR_SUCCESS) {
        if (err) {
            std::cerr << "EXR load error: " << err << std::endl;
            FreeEXRErrorMessage(err);
        }
        throw std::runtime_error("Background load failed");
    }
}

Vec3 Background::sample(const Vec3 &dir) const {
    const double rPI = 1/3.14159265359;
    int x = static_cast<int>((atan2(dir.z, dir.x)*rPI+1.5)*0.5 * width)%width;
    int y = static_cast<int>((asin(dir.y)*rPI+0.5) * height)%height;
    int index = (y*width + x)*channels;
    return Vec3(img[index], img[index+1], img[index+2]);
}