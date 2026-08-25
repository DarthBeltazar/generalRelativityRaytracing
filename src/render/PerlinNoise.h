#pragma once
#include <vector>

class PerlinNoise {
private:
    std::vector<int> p;

    static double fade(double t);
    static double lerp(double t, double a, double b);
    static double grad(int hash, double x, double y, double z);

public:
    PerlinNoise();
    explicit PerlinNoise(unsigned int seed);

    double noise(double x, double y, double z);
};

float fbm_noise(float x, float z, float time);