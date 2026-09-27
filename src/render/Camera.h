#pragma once
#include "core/Vec3.h"
#include "physics/physics.h"

struct CameraBasis {
    Vec3 forward, right, up;
    double focalLength = 1.0;
};

CameraBasis computeCameraBasis(double yaw, double pitch, double focalLength = 1.0);

double focalLengthToFov(double focalLength);
double fovToFocalLength(double fov);

Ray generateRay(int px, int py, double width, double height, double aspect, const Vec3 &origin,
                const CameraBasis &basis);