#pragma once
#include "core/Vec3.h"

#include <functional>
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

// Called after each accepted RKF45 step with (step index, distance to the black hole,
// step size h requested for that step). Used for diagnostics only - pass nullptr (the
// default) on the hot rendering path to skip it entirely.
using StepObserver = std::function<void(int step, double distance, double h)>;

HitInfo traceRay(double h, double rs, const Vec3 &bhpos, const Ray &ray, const StepObserver &observer = nullptr);