//
// Created by AlexandrGeorgiev on 29.08.2026.
//
#ifndef GR_SOURCE_DIR
#define GR_SOURCE_DIR "."
#endif
#include <SDL3/SDL.h>
#include <vector>

#include "render/Renderer.h"
#include <algorithm>
#include "core/Constants.h"
#include <cmath>
#include <cstdio>
#include <iostream>

int main(int, char**) {
    const int RW = 640;
    const int RH = 360;
    Background background;
    try {
        background.load(GR_SOURCE_DIR "/background.exr");
    } catch (std::runtime_error e) {
        background.load((std::string(SDL_GetBasePath()) + "background.exr").c_str());
    }
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("GeneralRelativityRaytracing - Viewer", 1920, 1080, SDL_WINDOW_RESIZABLE);
    SDL_Renderer *ren = SDL_CreateRenderer(win, nullptr);

    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, RW, RH);

    bool running = true;
    bool showMagnification = false;

    double focalLength = 1.0;
    const double zoomStep = 1.1;
    const double minFocalLength = fovToFocalLength(160 * PI / 180);
    const double maxFocalLength = fovToFocalLength(5 * PI / 180);
    auto updateTitle = [&]() {
        char title[128];
        std::snprintf(title, sizeof(title), "GeneralRelativityRaytracing - Viewer | FOV %.1f\xC2\xB0%s",
                      focalLengthToFov(focalLength) * 180 / PI, showMagnification ? " | magnification map" : "");
        SDL_SetWindowTitle(win, title);
    };
    updateTitle();

    double yaw = 0, pitch =- 0.04;
    Vec3 pos(0, -0.4, -5);
    const double sens = 0.00005;
    const double speed = 0.002;
    const auto t0 = std::chrono::high_resolution_clock::now();
    double dt = 0;
    auto prev = t0;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
            if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat && e.key.scancode == SDL_SCANCODE_M) {
                showMagnification = !showMagnification;
                updateTitle();
            }
            if (e.type == SDL_EVENT_MOUSE_WHEEL && e.wheel.y != 0) {
                focalLength = std::clamp(focalLength * std::pow(zoomStep, e.wheel.y), minFocalLength, maxFocalLength);
                updateTitle();
            }
            if (e.type == SDL_EVENT_MOUSE_MOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
                const double lookSens = sens / focalLength;
                yaw += e.motion.xrel * lookSens * dt;
                pitch -= e.motion.yrel * lookSens * dt;
                pitch = std::clamp(pitch, -PI*0.5 + 1e-3, PI*0.5 - 1e-3);
            }
        }

        const auto t1 = std::chrono::high_resolution_clock::now();
        dt = duration(prev, t1);
        const bool *keys = SDL_GetKeyboardState(nullptr);
        CameraBasis basis = computeCameraBasis(yaw, pitch, focalLength);
        if (keys[SDL_SCANCODE_W]) {
            pos = pos - basis.forward*dt*speed;
        }
        if (keys[SDL_SCANCODE_S]) {
            pos = pos + basis.forward*dt*speed;
        }
        if (keys[SDL_SCANCODE_A]) {
            pos = pos + basis.right*dt*speed;
        }
        if (keys[SDL_SCANCODE_D]) {
            pos = pos - basis.right*dt*speed;
        }
        if (keys[SDL_SCANCODE_Q]) {
            pos = pos + basis.up*dt*speed;
        }
        if (keys[SDL_SCANCODE_E]) {
            pos = pos - basis.up*dt*speed;
        }
        std::cout << 1000/dt << std::endl;
        prev = t1;
        const std::vector<HitInfo> his = traceRays(0.01, 0.5, RW, RH, pos, basis);
        std::vector<unsigned char> px = showMagnification
                                            ? shadeMagnification(his, RW, RH, basis)
                                            : shade(his, RW, RH, duration(t0, t1)*0.0003, background);
        SDL_UpdateTexture(tex, nullptr, px.data(), RW * 3);

        SDL_RenderClear(ren);
        SDL_RenderTexture(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);
    }
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);

    SDL_Quit();
    return 0;
}
