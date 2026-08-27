#include "Camera.h"

#include <cmath>

CameraBasis computeCameraBasis(const double yaw, const double pitch) {
    Vec3 forward(
        cos(pitch) * sin(yaw),
        sin(pitch),
        -cos(pitch) * cos(yaw));
    Vec3 right = forward.cross(Vec3(0, 1, 0)).normalize();
    Vec3 up = right.cross(forward);
    return CameraBasis{forward, right, up};
}

Ray generateRay(const int px, const int py, double width, double height, double aspect, const Vec3 &origin,
                const CameraBasis &basis) {
    double u = (2.0 * (px + 0.5) / width - 1.0) * aspect;
    double v = 1.0 - 2.0 * (py + 0.5) / height;
    Vec3 direction = (basis.right * u + basis.up * v + basis.forward).normalize();
    return Ray(origin, direction);
}