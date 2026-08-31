#include "Background.h"
#include "core/Constants.h"

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
    int x = static_cast<int>((atan2(dir.z, dir.x) * RECIP_PI + 1.5) * 0.5 * width) % width;
    int y = static_cast<int>((asin(dir.y) * RECIP_PI + 0.5) * height) % height;
    int index = (y * width + x) * channels;
    return Vec3(img[index], img[index + 1], img[index + 2]);
}

Vec3 Background::sampleAA(const double x0d, const double y0d, const double x1d, const double y1d) const {
    int x0 = x0d*width, y0 = y0d*height, x1 = x1d*width, y1 = y1d*height;
    if (x0 > x1) x1 += width;
    y0 = std::clamp(y0, 0, height-1);
    y1 = std::clamp(y1, 0, height-1);
    Vec3 out(0, 0, 0);
    int n = 0;
    for (int x = x0; x < x1+1; x++) {
        for (int y = y0; y < y1+1; y++) {
            int xw = (x%width + width) % width;
            int yh = std::clamp(y, 0, height-1);
            int index = (yh * width + xw) * channels;
            out = out + Vec3(img[index], img[index + 1], img[index + 2]);
            n++;
        }
    }
    return out*(1./n);
}
