//
// Created by Александр Георгиев on 30.07.2026.
//
#pragma once

class Vec3 {
public:
    double x, y, z;

    Vec3(double x, double y, double z);
    Vec3();
    Vec3 operator+(const Vec3 &other) const;
    Vec3 operator*(double mult) const;
    Vec3 operator-() const;
    Vec3 operator-(const Vec3& other) const;

    bool operator==(const Vec3 &other) const;

    double dot(const Vec3 &other) const;
    Vec3 cross(const Vec3 &other) const;
    double length() const;
    double squaredLength() const;
    Vec3 normalize() const;

    void print() const;
};
