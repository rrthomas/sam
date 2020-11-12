// -*- c -*-
// The traps.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include "minmax.h"

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"
#include "sam_traps.h"

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 128
#define PIXEL_SIZE 8
static SDL_Window *win;
static SDL_Renderer *ren;
static SDL_Surface *srf;

static void update_screen(void)
{
}

// Code adapted from https://www.thecrazyprogrammer.com/2017/01/bresenhams-line-drawing-algorithm-c-c.html
// Because SDL_RenderDrawLine's output is not pixel-scaled
static void drawline(int x1, int y1, int x2, int y2, uint8_t color)
{
    int x = MIN(x1, x2);
    int x_ = MAX(x1, x2);
    int y = MIN(y1, y2);
    int y_ = MAX(y1, y2);

    int dx = x_ - x;
    int dy = y_ - y;
    int p = 2 * dy - dx;

    if (dx == 0)
        vlineRGBA(ren, x1, y1, y2, color, color, color, 255);
    else if (dy == 0)
        hlineRGBA(ren, x1, x2, y1, color, color, color, 255);
    else {
        SDL_SetRenderDrawColor(ren, color, color, color, 255);
        while (x <= x_) {
            SDL_RenderDrawPoint(ren, x, y);
            if (p >= 0) {
                y++;
                p += 2 * dy - 2 * dx;
            } else
                p += 2 * dy;
            x++;
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
Uint32 getpixel(SDL_Surface *surface, int x, int y)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch * PIXEL_SIZE + x * bpp * PIXEL_SIZE;

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

    win = SDL_CreateWindow("SAM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DISPLAY_WIDTH * PIXEL_SIZE, DISPLAY_HEIGHT * PIXEL_SIZE, SDL_WINDOW_SHOWN);
    if (win == NULL)
        return SAM_ERROR_TRAP_INIT;

    srf = SDL_GetWindowSurface(win);
    ren = SDL_CreateSoftwareRenderer(srf);
    SDL_RenderSetLogicalSize(ren, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (ren == NULL)
        return SAM_ERROR_TRAP_INIT;

    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
    SDL_RenderClear(ren);
    SDL_UpdateWindowSurface(win);

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
        PUSH_INT(DISPLAY_WIDTH);
        break;
    case TRAP_DISPLAY_HEIGHT:
        PUSH_INT(DISPLAY_HEIGHT);
        break;
    case TRAP_CLEARSCREEN:
        {
            sam_word_t color;
            POP_UINT(color);
            SDL_SetRenderDrawColor(ren, color, color, color, 255);
            SDL_RenderClear(ren);
            SDL_UpdateWindowSurface(win);
        }
        break;
    case TRAP_SETDOT:
        {
            sam_word_t x, y, color;
            POP_UINT(color);
            POP_UINT(y);
            POP_UINT(x);
            SDL_SetRenderDrawColor(ren, color, color, color, 255);
            SDL_RenderDrawPoint(ren, x, y);
            SDL_UpdateWindowSurface(win);
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
            SDL_UpdateWindowSurface(win);
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
            SDL_SetRenderDrawColor(ren, color, color, color, 255);
            SDL_Rect rect = { .x = x, .y = y, .w = width, .h = height };
            SDL_RenderDrawRect(ren, &rect);
            SDL_UpdateWindowSurface(win);
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
            roundedRectangleRGBA(ren, x, y, x + width - 1, y + height - 1, radius, color, color, color, 255);
            SDL_UpdateWindowSurface(win);
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
            SDL_SetRenderDrawColor(ren, color, color, color, 255);
            SDL_Rect rect = { .x = x, .y = y, .w = width, .h = height };
            SDL_RenderFillRect(ren, &rect);
            SDL_UpdateWindowSurface(win);
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
                    Uint8 color = (Uint8)getpixel(srf, i, j);
                    SDL_SetRenderDrawColor(ren, ~color, ~color, ~color, 255);
                    SDL_RenderDrawPoint(ren, i, j);
                }
            SDL_UpdateWindowSurface(win);
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
            roundedRectangleRGBA(ren, xCenter - radius, yCenter - radius, xCenter + radius, yCenter + radius, radius, color, color, color, 255);
            SDL_UpdateWindowSurface(win);
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
            SDL_UpdateWindowSurface(win);
        }
        break;
    case TRAP_DRAWBITMAP:
        {
            sam_word_t bitmap, x, y, color;
            POP_UINT(color);
            POP_UINT(y);
            POP_UINT(x);
            POP_UINT(bitmap);
            SDL_SetRenderDrawColor(ren, color, color, color, 255);
            // TODO
            SDL_UpdateWindowSurface(win);
        }
        break;
    default:
        error = SAM_ERROR_INVALID_FUNCTION;
        break;
    }

 error:
    return error;
}
