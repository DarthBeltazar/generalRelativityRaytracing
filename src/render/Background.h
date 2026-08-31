#pragma once
#include "core/Vec3.h"

class Background {
public:
    ~Background();

    void load(const char *path);

    Vec3 sample(const Vec3 &dir) const;

    Vec3 sampleAA(double x0, double y0, double x1, double y1) const;

private:
    float *img = nullptr;
    int width = 0, height = 0;
    static constexpr int channels = 4;
};
