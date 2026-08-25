#pragma once
#include "core/Structs.h"
#include "physics/physics.h"
#include "render/Background.h"

#include <chrono>
#include <vector>

std::vector<HitInfo> traceRays(double h, double rs, int width, int height, const Vec3 &bhpos, double yaw, double pitch);

void writeImage(std::vector<HitInfo> his, int WIDTH, int HEIGHT, const char *filename, double time, const Background &background);

void renderImage(int WIDTH, int HEIGHT, const char *filename, double h, double rs, const Vec3 &bhpos, double yaw, double pitch, double time, const Background &background);

double duration(std::chrono::high_resolution_clock::time_point t1, std::chrono::high_resolution_clock::time_point t2);