#include "AccretionDisc.h"
#include "PerlinNoise.h"

#include <algorithm>
#include <cmath>

Vec3 discColor(Vec3 pos, double time) {
    double r = pos.length();
    double angle = atan2(pos.z,pos.x);

    const float R_IN = 1.5f;
    const float R_OUT = 4.5;

    if (r < R_IN || r > R_OUT) return {0.0f, 0.0f, 0.0f};


    float angularVelocity = 1.5f / (r * std::sqrt(r));
    float twistedAngle = angle - time * angularVelocity;

    float scale =.5f;
    float noiseX = r * std::cos(twistedAngle) * scale;
    float noiseZ = r * std::sin(twistedAngle) * scale;

    float plasmaDensity = fbm_noise(noiseX, noiseZ, time * 0.1f);

    float distanceFactor = (R_OUT - r) / (R_OUT - R_IN);
    float baseTemperature = std::pow(distanceFactor, 2.0f) * (R_IN / r);

    float currentTemperature = baseTemperature * (0.2f + 0.8f * plasmaDensity);


    Vec3 finalColor = {0.0f, 0.0f, 0.0f};

    if (currentTemperature > 0.05f) {
        finalColor.x = std::clamp(currentTemperature * 3.0f, 0.0f, 1.0f);
        finalColor.y = std::clamp((currentTemperature - 0.2f) * 2.5f, 0.0f, 1.0f);
        finalColor.z = std::clamp((currentTemperature - 0.6f) * 4.0f, 0.0f, 1.0f);
    }
    return finalColor;
}