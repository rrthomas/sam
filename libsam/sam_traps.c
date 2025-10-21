// The traps.
//
// (c) Reuben Thomas 1994-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <stdbool.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include "minmax.h"

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"
#include "sam_traps.h"

const unsigned sam_update_interval = 10; // milliseconds between screen updates

#define PIXEL_SIZE 8
static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Surface *srf;
static Uint64 last_update_time;
static bool need_window = false;

void sam_update_screen(void)
{
    SDL_ShowWindow(win);
    SDL_UpdateWindowSurface(win);
    SDL_RenderPresent(ren);
}

int sam_traps_process_events(void)
{
    SDL_bool quit = 0;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                if (win != NULL) {
                    SDL_DestroyWindow(win);
                    win = NULL;
                    quit = 1;
                }
            }
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                quit = 1;
                break;
            }
            break;
        }
    }

    if (need_window) {
        Uint64 now = SDL_GetTicks64();
        if (now - last_update_time > sam_update_interval) {
            last_update_time = now;
            sam_update_screen();
        }
    }

    return quit;
}

int sam_traps_window_used(void)
{
    return SDL_GetWindowFlags(win) & SDL_WINDOW_SHOWN;
}

// Code adapted from https://www.thecrazyprogrammer.com/2017/01/bresenhams-line-drawing-algorithm-c-c.html
// Because SDL_RenderDrawLine's output is not pixel-scaled
static void drawline(int x1, int y1, int x2, int y2, uint8_t color)
{
    if (x1 == x2)
        vlineRGBA(ren, x1, y1, y2, color, color, color, SDL_ALPHA_OPAQUE);
    else if (y1 == y2)
        hlineRGBA(ren, x1, x2, y1, color, color, color, SDL_ALPHA_OPAQUE);
    else {
        int dx = abs(x1 - x2);
        int dy = abs(y1 - y2);

        SDL_SetRenderDrawColor(ren, color, color, color, SDL_ALPHA_OPAQUE);
        if (dx >= dy) {
            int p = 2 * dy - dx;
            int x, x_, y, y_;
            if (x1 < x2) {
                x = x1; y = y1;
                x_ = x2; y_ = y2;
            } else {
                x = x2; y = y2;
                x_ = x1; y_ = y1;
            }
            while (x <= x_) {
                SDL_RenderDrawPoint(ren, x, y);
                if (p >= 0) {
                    y += (y < y_) ? 1 : -1;
                    p += 2 * dy - 2 * dx;
                } else
                    p += 2 * dy;
                x++;
            }
        } else {
            int p = 2 * dx - dy;
            int x, x_, y, y_;
            if (y1 < y2) {
                x = x1; y = y1;
                x_ = x2; y_ = y2;
            } else {
                x = x2; y = y2;
                x_ = x1; y_ = y1;
            }
            while (y <= y_) {
                SDL_RenderDrawPoint(ren, x, y);
                if (p >= 0) {
                    x += (x < x_) ? 1 : -1;
                    p += 2 * dx - 2 * dy;
                } else
                    p += 2 * dx;
                y++;
            }
        }
    }
}

// Adapted from glcd.cpp to get pixel-perfect copy!
static void fillcircle(uint8_t xCenter, uint8_t yCenter, uint8_t radius, uint8_t color)
{
    drawline(xCenter, yCenter - radius, xCenter, yCenter + radius, color);

    int f = 1 - radius;
    int ddF_x = 1;
    int ddF_y = -2 * radius;
    uint8_t x = 0;
    uint8_t y = radius;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawline(xCenter + x, yCenter + y, xCenter + x, yCenter - y, color);
        drawline(xCenter - x, yCenter + y, xCenter - x, yCenter - y, color);
        drawline(xCenter + y, yCenter + x, y + xCenter, yCenter - x, color);
        drawline(xCenter - y, yCenter + x, xCenter - y, yCenter - x, color);
    }
}

// Code from https://stackoverflow.com/questions/53033971/how-to-get-the-color-of-a-specific-pixel-from-sdl-surface
uint32_t sam_getpixel(int x, int y)
{
    int bpp = srf->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)srf->pixels + y * srf->pitch * PIXEL_SIZE + x * bpp * PIXEL_SIZE;

    switch (bpp)
        {
        case 1:
            return *p;
            break;

        case 2:
            return *(Uint16 *)p;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                return p[0] << 16 | p[1] << 8 | p[2];
            else
                return p[0] | p[1] << 8 | p[2] << 16;
            break;

        case 4:
            return *(Uint32 *)p;
            break;

        default:
            return 0;       /* shouldn't happen, but avoids warnings */
        }
}

sam_word_t sam_traps_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return SAM_ERROR_TRAP_INIT;

    win = SDL_CreateWindow("SAM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SAM_DISPLAY_WIDTH * PIXEL_SIZE, SAM_DISPLAY_HEIGHT * PIXEL_SIZE, SDL_WINDOW_HIDDEN);
    if (win == NULL)
        return SAM_ERROR_TRAP_INIT;

    srf = SDL_GetWindowSurface(win);
    ren = SDL_CreateSoftwareRenderer(srf);
    SDL_RenderSetLogicalSize(ren, SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT);
    if (ren == NULL)
        return SAM_ERROR_TRAP_INIT;

    SDL_SetRenderDrawColor(ren, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(ren);

    last_update_time = 0;

    return SAM_ERROR_OK;
}

void sam_traps_finish(void)
{
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

sam_word_t sam_trap(sam_uword_t function)
{
    int error = SAM_ERROR_OK;
    switch (function) {
    case TRAP_BLACK:
        PUSH_INT(0);
        break;
    case TRAP_WHITE:
        PUSH_INT(0xffffff);
        break;
    case TRAP_DISPLAY_WIDTH:
        PUSH_INT(SAM_DISPLAY_WIDTH);
        break;
    case TRAP_DISPLAY_HEIGHT:
        PUSH_INT(SAM_DISPLAY_HEIGHT);
        break;
    case TRAP_CLEARSCREEN:
        {
            sam_word_t color;
            POP_UINT(color);
            SDL_SetRenderDrawColor(ren, color, color, color, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(ren);
            need_window = true;
        }
        break;
    case TRAP_SETDOT:
        {
            sam_word_t x, y, color;
            POP_UINT(color);
            POP_UINT(y);
            POP_UINT(x);
            SDL_SetRenderDrawColor(ren, color, color, color, SDL_ALPHA_OPAQUE);
            SDL_RenderDrawPoint(ren, x, y);
            need_window = true;
        }
        break;
    case TRAP_DRAWLINE:
        {
            sam_word_t x1, y1, x2, y2, color;
            POP_UINT(color);
            POP_UINT(y2);
            POP_UINT(x2);
            POP_UINT(y1);
            POP_UINT(x1);
            drawline(x1, y1, x2, y2, color);
            need_window = true;
        }
        break;
    case TRAP_DRAWRECT:
        {
            sam_word_t x, y, width, height, color;
            POP_UINT(color);
            POP_UINT(height);
            POP_UINT(width);
            POP_UINT(y);
            POP_UINT(x);
            SDL_SetRenderDrawColor(ren, color, color, color, SDL_ALPHA_OPAQUE);
            SDL_Rect rect = { .x = x, .y = y, .w = width, .h = height };
            SDL_RenderDrawRect(ren, &rect);
            need_window = true;
        }
        break;
    case TRAP_DRAWROUNDRECT:
        {
            sam_word_t x, y, width, height, radius, color;
            POP_UINT(color);
            POP_UINT(radius);
            POP_UINT(height);
            POP_UINT(width);
            POP_UINT(y);
            POP_UINT(x);
            roundedRectangleRGBA(ren, x, y, x + width - 1, y + height - 1, radius, color, color, color, SDL_ALPHA_OPAQUE);
            need_window = true;
        }
        break;
    case TRAP_FILLRECT:
        {
            sam_word_t x, y, width, height, color;
            POP_UINT(color);
            POP_UINT(height);
            POP_UINT(width);
            POP_UINT(y);
            POP_UINT(x);
            SDL_SetRenderDrawColor(ren, color, color, color, SDL_ALPHA_OPAQUE);
            SDL_Rect rect = { .x = x, .y = y, .w = width, .h = height };
            SDL_RenderFillRect(ren, &rect);
            need_window = true;
        }
        break;
    case TRAP_INVERTRECT:
        {
            sam_word_t x, y, width, height;
            POP_UINT(height);
            POP_UINT(width);
            POP_UINT(y);
            POP_UINT(x);
            sam_word_t i, j;
            for (i = x; i < x + width; i++)
                for (j = y; j < y + height; j++) {
                    Uint8 color = (Uint8)sam_getpixel(i, j);
                    SDL_SetRenderDrawColor(ren, ~color, ~color, ~color, SDL_ALPHA_OPAQUE);
                    SDL_RenderDrawPoint(ren, i, j);
                }
            need_window = true;
        }
        break;
    case TRAP_DRAWCIRCLE:
        {
            sam_word_t xCenter, yCenter, radius, color;
            POP_UINT(color);
            POP_UINT(radius);
            POP_UINT(yCenter);
            POP_UINT(xCenter);
            // Use roundedRectangle to match GLCD.
            roundedRectangleRGBA(ren, xCenter - radius, yCenter - radius, xCenter + radius, yCenter + radius, radius, color, color, color, SDL_ALPHA_OPAQUE);
            need_window = true;
        }
        break;
    case TRAP_FILLCIRCLE:
        {
            sam_word_t xCenter, yCenter, radius, color;
            POP_UINT(color);
            POP_UINT(radius);
            POP_UINT(yCenter);
            POP_UINT(xCenter);
            fillcircle(xCenter, yCenter, radius, color);
            need_window = true;
        }
        break;
    case TRAP_DRAWBITMAP:
        {
            sam_word_t bitmap, x, y, color;
            POP_UINT(color);
            POP_UINT(y);
            POP_UINT(x);
            POP_UINT(bitmap);
            SDL_SetRenderDrawColor(ren, color, color, color, SDL_ALPHA_OPAQUE);
            // TODO
            need_window = true;
        }
        break;
    default:
        error = SAM_ERROR_INVALID_FUNCTION;
        break;
    }

 error:
    return error;
}
