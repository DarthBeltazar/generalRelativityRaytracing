#pragma once
#include "core/Vec3.h"

#include <vector>

struct Ray {
    Vec3 origin, dir;

    Ray(Vec3 origin, Vec3 dir);

    Ray();
};

struct HitInfo {
    bool hit = false;
    bool discHit = false;
    double t = 0;
    std::vector<Vec3> pos;
    Vec3 dir;
};

HitInfo traceRay(double h, double rs, const Vec3 &bhpos, const Ray &ray);