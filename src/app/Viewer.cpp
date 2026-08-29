//
// Created by AlexandrGeorgiev on 29.08.2026.
//
#ifndef GR_SOURCE_DIR
#define GR_SOURCE_DIR "."
#endif
#include <SDL3/SDL.h>
#include <vector>

#include "render/Renderer.h"

int main(int, char**) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("GeneralRelativityRaytracing - Viewer", 960, 540, 0);
    SDL_Renderer *ren = SDL_CreateRenderer(win, nullptr);

    const int RW = 960;
    const int RH = 540;
    Background background;
    background.load(GR_SOURCE_DIR "/background.exr");

    SDL_Texture *tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, RW, RH);
    std::vector<unsigned char> px = shade(traceRays(0.01, 0.5, RW, RH, Vec3(0, -0.4, -5), 0, -0.04),
                                          RW, RH, 0.0, background);
    SDL_UpdateTexture(tex, nullptr, px.data(), RW * 3);

    bool running = true;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) running = false;
        }
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
