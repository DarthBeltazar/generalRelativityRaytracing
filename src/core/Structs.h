#pragma once
#include <cmath>
#include <functional>

class Vec3 {
public:
    double x, y, z;

    Vec3(double x, double y, double z) : x(x), y(y), z(z) {
    }

    Vec3() : x(0), y(0), z(0) {
    }

    Vec3 operator+(const Vec3 &other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator*(double mult) const { return Vec3(x * mult, y * mult, z * mult); }
    Vec3 operator-() const { return Vec3(-x, -y, -z); }
    Vec3 operator-(const Vec3 &other) const { return Vec3(x - other.x, y - other.y, z - other.z); }

    bool operator==(const Vec3 &other) const {
        constexpr double eps = 1e-9;
        return std::abs(x - other.x) + std::abs(y - other.y) + std::abs(z - other.z) < eps;
    }

    double dot(const Vec3 &other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    Vec3 cross(const Vec3 &other) const {
        return Vec3(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
    }

    double length() const {
        return std::sqrt(squaredLength());
    }

    double squaredLength() const {
        return dot(*this);
    }

    Vec3 normalize() const {
        return *this * (1.0 / this->length());
    }

    Vec3 custom(const std::function<double(double)> &f) const {
        return Vec3(f(x), f(y), f(z));
    }

    void print() const;
};

struct State {
    double u, w;
    State operator+(const State &other) const { return State(u + other.u, w + other.w); }
    State operator*(double d) const { return State(u * d, w * d); }

    State(double u, double w) : u(u), w(w) {
    }
};
