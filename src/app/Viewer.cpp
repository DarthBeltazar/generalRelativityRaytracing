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
            if (e.type == SDL_EVENT_MOUSE_MOTION && (e.motion.state & SDL_BUTTON_LMASK)) {
                yaw += e.motion.xrel * sens * dt;
                pitch -= e.motion.yrel * sens * dt;
                pitch = std::clamp(pitch, -PI*0.5, PI*0.5);
            }
        }

        const auto t1 = std::chrono::high_resolution_clock::now();
        dt = duration(prev, t1);
        const bool *keys = SDL_GetKeyboardState(nullptr);
        CameraBasis basis = computeCameraBasis(yaw, pitch);
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
        std::vector<unsigned char> px = shade(traceRays(0.01, 0.5, RW, RH, pos, basis),
                                              RW, RH, duration(t0, t1)*0.0003, background);
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
