/**
 * https://github.com/deltabeard/sdl2_picofont
 *
 * Font rendering library for SDL2.
 * Copyright (C) 2020  Mahyar Koshkouei
 * Copyright (C) 2024  Beyley Thomas <ep1cm1n10n123@gmail.com>
 *
 * This is free and unencumbered software released into the public domain.
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 * For more information, please refer to <https://unlicense.org>
 */

#pragma once

#include <SDL2/SDL.h>

#define FONT_CHAR_WIDTH 9
#define FONT_CHAR_HEIGHT 15

/**
 * Basic process of using SDL2_Picofont to draw text onto a renderer:
 *
 * ctx = FontStartup(renderer);
 * FontPrintToRenderer(ctx, "Text to draw", NULL);
 * FontExit(ctx);
 *
 * If your platform supports it, you can set your render target to a texture,
 * and then call FontPrintToRenderer() to print text to the texture instead.
 * Use the function FontDrawSize() to ensure that your texture is large enough.
 */

/**
 * Context required to store the generated texture with the given renderer.
 */
// typedef struct font_ctx_s font_ctx;

typedef struct font_ctx
{
    SDL_Texture *tex;
    SDL_Renderer *rend;
} font_ctx;

/**
 * Initialises font context with given renderer. Must be called first.
 * The given renderer must remain valid until after FontExit() is called.
 *
 * \param renderer	Renderer of the window.
 * \return Font context, else error. Use SDL_GetError().
 */
font_ctx *font_startup(SDL_Renderer *renderer);

/**
 * Prints a string to the SDL2 renderer.
 * Use SDL_SetRenderDrawColor() to change the text colour.
 * Use SDL_SetRenderTarget() to render to a texture instead.
 *
 * \param ctx	Font library context.
 * \param text	Text to print.
 * \param dstscale	Location and scale of text. (0,0) x1 if NULL.
 * \return	0 on success, else error. Use SDL_GetError().
 */
int font_print_to_renderer(font_ctx *const ctx, const char *text,
                        const SDL_Rect *dstscale);

/**
 * Expected size of texture/renderer required given the string length, assuming
 * a scale of 1.
 *
 * \param text_len	Length of text.
 * \param w		Pointer to store expected width.
 * \param h		Pointer to store expected height.
 */
void FontDrawSize(const unsigned text_len, unsigned *w, unsigned *h);

/**
 * Deletes font context.
 */
void font_exit(font_ctx *ctx);