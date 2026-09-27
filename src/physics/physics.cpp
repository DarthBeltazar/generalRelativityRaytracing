#include "physics.h"
#include "core/Constants.h"

#include <algorithm>
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

    State rkf45Step(const State &y, double rs, double &h, double &hUsed, double atol, double rtol) {
        constexpr double s = 0.84;
        constexpr double hMin = 1e-4;
        constexpr double hMax = 0.5;
        h = std::clamp(h, hMin, hMax);
        while (true) {
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

            State error = k1 * (1.0 / 360.0)
                        + k3 * (-128.0 / 4275.0)
                        + k4 * (-2197.0 / 75240.0)
                        + k5 * (1.0 / 50.0)
                        + k6 * (2.0 / 55.0);
            const double tol_u = atol + rtol * std::abs(y_next.u);
            const double tol_w = atol + rtol * std::abs(y_next.w);
            const double err_norm = std::max(std::abs(error.u)/tol_u, std::abs(error.w)/tol_w);

            hUsed = h;
            const double h_opt = s * h * std::pow(1 / (err_norm + 1e-15), 0.2);
            h = std::clamp(h * std::max(0.1, std::min(4.0, h_opt / h)), hMin, hMax);
            if (!(err_norm > 1) || hUsed <= hMin) {
                return y_next;
            }
        }
    }

    template<typename T>
    int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }
}

HitInfo traceRay(const double h0, const double rs, const Vec3 &bhpos, const Ray &ray, const StepObserver &observer) {
    Vec3 r_vec = ray.origin - bhpos;
    double r_cam = r_vec.length();
    double b0s = r_vec.cross(ray.dir).squaredLength();
    double bbs = (1 - rs / r_cam) / b0s;
    double u0 = 1 / r_cam;

    State y(u0, -sqrt(rs * u0 * u0 * u0 - u0 * u0 + bbs) * sgn(r_vec.dot(ray.dir)));

    Vec3 e_t = r_vec.cross(ray.dir).cross(r_vec).normalize();
    Vec3 e_r = r_vec.normalize();

    auto directionAt = [&](const double cosPhi, const double sinPhi) {
        return e_r * cosPhi + e_t * sinPhi;
    };
    auto rotate = [](double &cosPhi, double &sinPhi, const double da) {
        const double c = std::cos(da), s = std::sin(da);
        const double cosNew = cosPhi * c - sinPhi * s;
        sinPhi = sinPhi * c + cosPhi * s;
        cosPhi = cosNew;
    };

    //sin and cos of sum optimization - faster than calculate them for each state
    double cosI = 1.0;
    double sinI = 0.0;

    HitInfo hi;
    int steps = 0;
    State yPrev = y;
    double prevCosI = cosI;
    double prevSinI = sinI;
    auto h = h0;
    for (int i = 0; i < 1000; i++) {
        prevCosI = cosI;
        prevSinI = sinI;
        yPrev = y;
        const double h_requested = h;
        double hStep;
        y = rkf45Step(y, rs, h, hStep, 1e-7, 1e-7);

        double cosH, sinH;
        if (hStep < 0.1) {
            cosH = 1 - 0.5 * hStep * hStep;
            sinH = hStep - 1./6 * hStep * hStep * hStep;
        }
        else {
            cosH = cos(hStep);
            sinH = sin(hStep);
        }
        cosI = prevCosI * cosH - prevSinI * sinH;
        sinI = prevSinI * cosH + prevCosI * sinH;

        steps++;
        if (observer) {
            observer(steps, 1 / std::abs(y.u), h_requested);
        }


        if (yPrev.u > 0 && y.u > 0) {
            const double gPrev = e_r.y * prevCosI + e_t.y * prevSinI;
            const double gCur = e_r.y * cosI + e_t.y * sinI;
            if (gPrev * gCur < 0) {
                const double dgPrev = -e_r.y * prevSinI + e_t.y * prevCosI;

                double a = std::atan2(gPrev, -dgPrev);
                if (a <= 0) a += PI;
                const double t = std::clamp(a / hStep, 0.0, 1.0);
                const double t2 = t * t, t3 = t2 * t;
                const double u = (2 * t3 - 3 * t2 + 1) * yPrev.u + (t3 - 2 * t2 + t) * hStep * yPrev.w
                               + (-2 * t3 + 3 * t2) * y.u + (t3 - t2) * hStep * y.w;
                double cosC = prevCosI, sinC = prevSinI;
                rotate(cosC, sinC, a);
                hi.pos.push_back(directionAt(cosC, sinC) * (1 / u));
                hi.discHit = true;
            }
        }

        if (y.u >= 1 / rs || y.u <= 0) {
            break;
        }
    }

    hi.hit = y.u >= 1 / rs;
    if (!hi.hit) {
        const bool usePrev = std::abs(yPrev.u) < std::abs(y.u);
        const State &yEnd = usePrev ? yPrev : y;
        double cosE = usePrev ? prevCosI : cosI;
        double sinE = usePrev ? prevSinI : sinI;
        if (yEnd.w < 0) {
            rotate(cosE, sinE, std::atan2(yEnd.u, -yEnd.w));
            hi.dir = directionAt(cosE, sinE);
        } else {
            hi.dir = (directionAt(-sinE, cosE) * yEnd.u - directionAt(cosE, sinE) * yEnd.w).normalize();
        }
    }
    hi.t = steps + 1;
    return hi;
}
