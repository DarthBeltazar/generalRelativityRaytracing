#pragma once
#include "core/Vec3.h"
#include "physics/physics.h"

struct CameraBasis {
    Vec3 forward, right, up;
};

CameraBasis computeCameraBasis(double yaw, double pitch);

Ray generateRay(int px, int py, double width, double height, double aspect, const Vec3 &origin,
                const CameraBasis &basis);