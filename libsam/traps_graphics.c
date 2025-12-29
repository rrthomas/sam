// SAM's graphics traps.
//
// (c) Reuben Thomas 2020-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <stdbool.h>

#include <SDL.h>
#include <SDL2_gfxPrimitives.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "private.h"
#include "run.h"
#include "traps_graphics.h"

const unsigned sam_update_interval = 10; // milliseconds between screen updates

#define PIXEL_SIZE 2
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

int sam_graphics_process_events(void)
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

int sam_graphics_window_used(void)
{
    return SDL_GetWindowFlags(win) & SDL_WINDOW_SHOWN;
}

// Code from https://stackoverflow.com/questions/53033971/how-to-get-the-color-of-a-specific-pixel-from-sdl-surface
uint32_t sam_getpixel(int x, int y)
{
    int bpp = srf->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)srf->pixels + y * srf->pitch * PIXEL_SIZE + x * bpp * PIXEL_SIZE;

    switch (bpp) {
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
        return 0; /* shouldn't happen, but avoids warnings */
    }
}

sam_word_t sam_graphics_init(void)
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

void sam_graphics_finish(void)
{
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

sam_word_t sam_graphics_trap(sam_stack_t *s, sam_uword_t function)
{
    int error = SAM_ERROR_OK;
    switch (function) {
    case TRAP_GRAPHICS_BLACK:
        PUSH_INT(0);
        break;
    case TRAP_GRAPHICS_WHITE:
        PUSH_INT(-1);
        break;
    case TRAP_GRAPHICS_DISPLAY_WIDTH:
        PUSH_INT(SAM_DISPLAY_WIDTH);
        break;
    case TRAP_GRAPHICS_DISPLAY_HEIGHT:
        PUSH_INT(SAM_DISPLAY_HEIGHT);
        break;
    case TRAP_GRAPHICS_CLEARSCREEN:
        {
            sam_word_t color;
            POP_UINT(color);
            SDL_SetRenderDrawColor(ren, color, color, color, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(ren);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_SETDOT:
        {
            sam_word_t x, y, color;
            POP_UINT(color);
            POP_UINT(y);
            POP_UINT(x);
            pixelRGBA(ren, x, y, color, color, color, SDL_ALPHA_OPAQUE);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_DRAWLINE:
        {
            sam_word_t x1, y1, x2, y2, color;
            POP_UINT(color);
            POP_UINT(y2);
            POP_UINT(x2);
            POP_UINT(y1);
            POP_UINT(x1);
            lineRGBA(ren, x1, y1, x2, y2, color, color, color, SDL_ALPHA_OPAQUE);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_DRAWRECT:
        {
            sam_word_t x, y, width, height, color;
            POP_UINT(color);
            POP_UINT(height);
            POP_UINT(width);
            POP_UINT(y);
            POP_UINT(x);
            rectangleRGBA(ren, x, y, x + width - 1, y + height - 1, color, color, color, SDL_ALPHA_OPAQUE);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_DRAWROUNDRECT:
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
    case TRAP_GRAPHICS_FILLRECT:
        {
            sam_word_t x, y, width, height, color;
            POP_UINT(color);
            POP_UINT(height);
            POP_UINT(width);
            POP_UINT(y);
            POP_UINT(x);
            boxRGBA(ren, x, y, x + width - 1, y + height - 1, color, color, color, SDL_ALPHA_OPAQUE);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_DRAWCIRCLE:
        {
            sam_word_t xCenter, yCenter, radius, color;
            POP_UINT(color);
            POP_UINT(radius);
            POP_UINT(yCenter);
            POP_UINT(xCenter);
            circleRGBA(ren, xCenter, yCenter, radius, color, color, color, SDL_ALPHA_OPAQUE);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_FILLCIRCLE:
        {
            sam_word_t xCenter, yCenter, radius, color;
            POP_UINT(color);
            POP_UINT(radius);
            POP_UINT(yCenter);
            POP_UINT(xCenter);
            filledCircleRGBA(ren, xCenter, yCenter, radius, color, color, color, SDL_ALPHA_OPAQUE);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_DRAWBITMAP:
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
        error = SAM_ERROR_INVALID_TRAP;
        break;
    }

 error:
    return error;
}

void sam_graphics_wait(void) {
    while (sam_graphics_process_events() == 0) {
        SDL_Delay(sam_update_interval);
    }
}

char *sam_graphics_trap_name(sam_word_t function) {
    switch (function) {
    case TRAP_GRAPHICS_BLACK:
        return "BLACK";
    case TRAP_GRAPHICS_WHITE:
        return "WHITE";
    case TRAP_GRAPHICS_DISPLAY_WIDTH:
        return "DISPLAY_WIDTH";
    case TRAP_GRAPHICS_DISPLAY_HEIGHT:
        return "DISPLAY_HEIGHT";
    case TRAP_GRAPHICS_CLEARSCREEN:
        return "CLEARSCREEN";
    case TRAP_GRAPHICS_SETDOT:
        return "SETDOT";
    case TRAP_GRAPHICS_DRAWLINE:
        return "DRAWLINE";
    case TRAP_GRAPHICS_DRAWRECT:
        return "DRAWRECT";
    case TRAP_GRAPHICS_DRAWROUNDRECT:
        return "DRAWROUNDRECT";
    case TRAP_GRAPHICS_FILLRECT:
        return "FILLRECT";
    case TRAP_GRAPHICS_DRAWCIRCLE:
        return "DRAWCIRCLE";
    case TRAP_GRAPHICS_FILLCIRCLE:
        return "FILLCIRCLE";
    case TRAP_GRAPHICS_DRAWBITMAP:
        return "DRAWBITMAP";
    default:
        return NULL;
    }
}
