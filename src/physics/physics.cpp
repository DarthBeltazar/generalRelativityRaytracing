#include "physics.h"

#include <cmath>

Ray::Ray(Vec3 origin, Vec3 dir) : origin(origin), dir(dir) {
}

Ray::Ray() : origin(Vec3()), dir(Vec3()) {
}

namespace {
    State f(const State &state, double rs) {
        return State(state.w, -state.u + 1.5 * rs * state.u * state.u);
    }

    State rk4Step(State y, double rs, double h) {
        State k1 = f(y, rs);
        State k2 = f(y + k1 * (h / 2), rs);
        State k3 = f(y + k2 * (h / 2), rs);
        State k4 = f(y + k3 * h, rs);
        State y_next = y + (k1 + k2 + k2 + k3 + k3 + k4) * (h / 6);
        return y_next;
    }

    template<typename T>
    int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }
}

HitInfo traceRay(const double h, const double rs, const Vec3 &bhpos, const Ray &ray) {
    Vec3 r_vec = ray.origin - bhpos;
    double r_cam = r_vec.length();
    double b0s = r_vec.cross(ray.dir).squaredLength();
    double bbs = (1 - rs / r_cam) / b0s;
    double u0 = 1 / r_cam;

    State y(u0, -sqrt(rs * u0 * u0 * u0 - u0 * u0 + bbs) * sgn(r_vec.dot(ray.dir)));

    Vec3 e_t = r_vec.cross(ray.dir).cross(r_vec).normalize();
    Vec3 e_r = r_vec.normalize();

    //sin and cos of sum optimization - faster than calculate them for each state
    const double cosH = cos(h);
    const double sinH = sin(h);
    double cosI = 1.0;
    double sinI = 0.0;
    auto positionAt = [&](const State &s) {
        return (e_r * cosI + e_t * sinI) * (1 / s.u);
    };

    HitInfo hi;
    int steps = 0;
    Vec3 prev = positionAt(y);
    Vec3 prevPrev = prev;
    Vec3 current = prev;
    for (int i = 0; i < 1000; i++) {
        y = rk4Step(y, rs, h);
        steps++;
        prevPrev = current;
        const double nextCosI = cosI * cosH - sinI * sinH;
        const double nextSinI = sinI * cosH + cosI * sinH;
        cosI = nextCosI;
        sinI = nextSinI;
        current = positionAt(y);

        if (current.y * prev.y < 0) {
            Vec3 delta = current - prev;
            hi.pos.push_back(prev - delta * prev.y * (1. / delta.y));
            hi.discHit = true;
        }
        prev = current;

        if (y.u >= 1 / rs || y.u <= 0) {
            break;
        }
    }

    hi.hit = y.u >= 1 / rs;
    if (!hi.hit) {
        hi.dir = (current - prevPrev).normalize();
    }
    hi.t = steps + 1;
    return hi;
}
