#include <SDL2/SDL.h>
#include <stdbool.h>
#include <lv2/spu.h>

#include "sdl2_picofont.h"

const SDL_Colour white = {.r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF};
const SDL_Colour black = {.r = 0x00, .g = 0x00, .b = 0x00, .a = 0xFF};
const SDL_Colour skyblue = {.r = 0x87, .g = 0xCE, .b = 0xEB, .a = 0xFF};

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        *((char *)1) = 1;
    }

    // *((char *)10) = 1;

    // if (SDL_VideoInit(NULL) != 0)
    // {
    //     SDL_Log("Unable to initialize SDL Video: %s", SDL_GetError());
    //     *((char *)2) = 1;
    // }

    // *((char *)11) = 1;

    SDL_Window *window = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == NULL)
    {
        SDL_Log("Unable to create window: %s", SDL_GetError());
        *((char *)3) = 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);

    font_ctx *font = FontStartup(renderer);

    SDL_Event ev;
    bool quit = false;

    while (!quit)
    {
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
            {
                quit = true;
                // *((char *)4) = 1;
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect dstscale = {.x = 10, .y = 10, .h = 4, .w = 3};

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        FontPrintToRenderer(font, "Refresher", &dstscale);

        dstscale.y += FONT_CHAR_HEIGHT * dstscale.h;

        dstscale.w = 1;
        dstscale.h = 1;

        FontPrintToRenderer(font, "LittleBigPlanet (NPEA00241)", &dstscale);
        dstscale.y += FONT_CHAR_HEIGHT * dstscale.h;

        FontPrintToRenderer(font, ">>> LittleBigPlanet2 Digital Version (NPUA80662) <<<", &dstscale);
        dstscale.y += FONT_CHAR_HEIGHT * dstscale.h;

        FontPrintToRenderer(font, "LittleBigPlanet3 Digital Version (NPUA81116)", &dstscale);
        dstscale.y += FONT_CHAR_HEIGHT * dstscale.h;

        SDL_RenderPresent(renderer);
    }

    FontExit(font);
    SDL_Quit();

    return 0;
}