#include "physics.h"

#include <cmath>

Ray::Ray(Vec3 origin, Vec3 dir) : origin(origin), dir(dir) {
}

Ray::Ray() : origin(Vec3()), dir(Vec3()) {
}

namespace {
    struct State {
        double u, w;
        State operator+(const State &other) const { return State(u + other.u, w + other.w); }
        State operator*(double d) const { return State(u * d, w * d); }

        State(double u, double w) : u(u), w(w) {
        }
    };

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

    template<typename F>
    State rkf45Step(State y, double rs, double &h, double tol, F accept) {
        bool isDisaccepted = false;
        while (true) {
            const double s = 0.84;
            State k1 = f(y, rs) * h;

            State k2 = f(y + k1 * (1.0 / 4.0), rs) * h;

            State k3 = f(y + k1 * (3.0 / 32.0) + k2 * (9.0 / 32.0), rs) * h;

            State k4 = f(y + k1 * (1932.0 / 2197.0) + k2 * (-7200.0 / 2197.0) + k3 * (7296.0 / 2197.0),
                         rs) * h;

            State k5 = f(y + k1 * (439.0 / 216.0) + k2 * -8.0 + k3 * (3680.0 / 513.0) + k4 * (-845.0 / 4104.0),
                         rs) * h;

            State k6 = f(y + k1 * (-8.0 / 27.0) + k2 * 2.0 + k3 * (-3544.0 / 2565.0) + k4 * (1859.0 / 4104.0) + k5 * (-11.0 / 40.0),
                         rs) * h;


            State y_next = y + k1 * (16.0 / 135.0)
                                        + k3 * (6656.0 / 12825.0)
                                        + k4 * (28561.0 / 56430.0)
                                        + k5 * (-9.0 / 50.0)
                                        + k6 * (2.0 / 55.0);

            bool currentAccept = accept(y_next);
            isDisaccepted = !currentAccept || isDisaccepted;
            if (!isDisaccepted) {
                State error = k1 * (1.0 / 360.0)
                            + k3 * (-128.0 / 4275.0)
                            + k4 * (-2197.0 / 75240.0)
                            + k5 * (1.0 / 50.0)
                            + k6 * (2.0 / 55.0);
                double err_norm = std::max(abs(error.u), abs(error.w));

                double h_opt = s * h * std::pow(tol / (err_norm + 1e-15), 0.25);
                h = h * std::max(0.1, std::min(4.0, h_opt / h));
                if (err_norm <= tol) {
                    return y_next;
                }
            } else {
                if (!currentAccept) {
                    h *= 0.5;
                } else {
                    return y_next;
                }
            }
        }
    }

    template<typename T>
    int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }
}

HitInfo traceRay(const double h0, const double rs, const Vec3 &bhpos, const Ray &ray) {
    Vec3 r_vec = ray.origin - bhpos;
    double r_cam = r_vec.length();
    double b0s = r_vec.cross(ray.dir).squaredLength();
    double bbs = (1 - rs / r_cam) / b0s;
    double u0 = 1 / r_cam;

    State y(u0, -sqrt(rs * u0 * u0 * u0 - u0 * u0 + bbs) * sgn(r_vec.dot(ray.dir)));

    Vec3 e_t = r_vec.cross(ray.dir).cross(r_vec).normalize();
    Vec3 e_r = r_vec.normalize();

    //sin and cos of sum optimization - faster than calculate them for each state
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
    auto h = h0;
    for (int i = 0; i < 1000; i++) {
        double prevCosI = cosI;
        double prevSinI = sinI;
        prevPrev = current;
        y = rkf45Step(y, rs, h, 1e-7, [&](const State &s) -> bool {
            const double cosH = 1 - 0.5 * h * h;
            const double sinH = h - 1./6 * h * h * h;
            cosI = prevCosI * cosH - prevSinI * sinH;
            sinI = prevSinI * cosH + prevCosI * sinH;
            current = positionAt(s);
            return (current.y * prev.y > -current.squaredLength()/(rs*rs));
        });
        steps++;

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
