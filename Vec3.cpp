//
// Created by Александр Георгиев on 30.07.2026.
//

#include "Vec3.h"
#include <cmath>
#include <iostream>

Vec3::Vec3(double x, double y, double z) : x(x), y(y), z(z) {}
Vec3::Vec3() : x(0), y(0), z(0) {}
Vec3 Vec3::operator+(const Vec3 &other) const {return Vec3(x + other.x, y + other.y, z + other.z);}
Vec3 Vec3::operator*(double mult) const {return Vec3(x * mult, y * mult, z * mult);}
Vec3 Vec3::operator-() const {return Vec3(-x, -y, -z);}
Vec3 Vec3::operator-(const Vec3 &other) const {return Vec3(x - other.x, y - other.y, z - other.z);}
bool Vec3::operator==(const Vec3 &other) const {
    const double eps = 1e-9;
    return abs(x - other.x) + abs(y - other.y) + abs(z - other.z) < eps;
}

double Vec3::dot(const Vec3 &other) const {
    return x * other.x + y * other.y + z * other.z;
}

Vec3 Vec3::cross(const Vec3 &other) const {
    return Vec3(y * other.z - z * other.y,  z * other.x - x * other.z,  x * other.y - y * other.x);
}

double Vec3::length() const {
    return sqrt(this->dot(*this));
}

double Vec3::squaredLength() const {
    return this->dot(*this);
}

Vec3 Vec3::normalize() const {
    return *this * (1.0 / this->length());
}

void Vec3::print() const {
    std::cout << "X: " << x << "  Y: " << y << "  Z: " << z << std::endl;
}
