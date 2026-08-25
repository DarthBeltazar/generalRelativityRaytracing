#pragma once
#include "core/Structs.h"

class Background {
public:
    ~Background();

    void load(const char *path);

    Vec3 sample(const Vec3 &dir) const;

private:
    float *img = nullptr;
    int width = 0, height = 0;
    static constexpr int channels = 4;
};
