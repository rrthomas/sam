// SAM's graphics traps.
//
// (c) Reuben Thomas 2020-2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <stdbool.h>

#include <SDL.h>

#ifndef DEBUG_NANOVG
#define NVG_LOG(...)
#endif
#include "nanovg.h"
#define NANOVG_SW_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#include "nanovg_sw.h"
#pragma GCC diagnostic pop

#include "sam.h"
#include "sam_opcodes.h"
#include "private.h"
#include "run.h"
#include "traps_graphics.h"

const unsigned sam_update_interval = 10; // milliseconds between screen updates

#define PIXEL_SIZE 2 // FIXME calculate pixel ratio
static SDL_Window *win;
static SDL_Surface *srf;
static Uint64 last_update_time;
static bool need_window = false;
static NVGcontext* vg;
static SDL_PixelFormat *fmt;
const unsigned bytes_per_pixel = 4;

#define POP_COLOR(var)                                   \
    do {                                                 \
        sam_uword_t color_word;                          \
        POP_UINT(color_word);                            \
        var = (NVGcolor){.c = (unsigned int)color_word}; \
    } while (0)

void sam_update_screen(void)
{
    SDL_ShowWindow(win);
    SDL_UpdateWindowSurface(win);
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

// Code adapted from https://stackoverflow.com/questions/53033971/how-to-get-the-color-of-a-specific-pixel-from-sdl-surface
uint32_t sam_getpixel(int x, int y)
{
    Uint32 *p = (Uint32 *)((Uint8 *)srf->pixels + y * srf->pitch * PIXEL_SIZE + x * bytes_per_pixel * PIXEL_SIZE);
    return *p;
}

sam_word_t sam_graphics_init(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return SAM_ERROR_TRAP_INIT;

    win = SDL_CreateWindow("SAM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SAM_DISPLAY_WIDTH * PIXEL_SIZE, SAM_DISPLAY_HEIGHT * PIXEL_SIZE, SDL_WINDOW_HIDDEN);
    if (win == NULL)
        return SAM_ERROR_TRAP_INIT;

    srf = SDL_GetWindowSurface(win);
    vg = nvgswCreate(NVG_SRGB | NVG_AUTOW_DEFAULT);
    fmt = srf->format;
    nvgswSetFramebuffer(vg, srf->pixels, srf->w, srf->h, fmt->Rshift, fmt->Gshift, fmt->Bshift, 24);

    last_update_time = 0;

    return SAM_ERROR_OK;
}

void sam_graphics_finish(void)
{
    SDL_DestroyWindow(win);
    SDL_Quit();
}

sam_word_t sam_graphics_trap(sam_state_t *state, sam_uword_t function)
{
#define s ((sam_stack_t *)state->s0->data)
    int error = SAM_ERROR_OK;

    switch (function) {
    case TRAP_GRAPHICS_BLACK:
        PUSH_INT(nvgRGBA(0, 0, 0, 255).c);
        break;
    case TRAP_GRAPHICS_WHITE:
        PUSH_INT(nvgRGBA(255, 255, 255, 255).c);
        break;
    case TRAP_GRAPHICS_DISPLAY_WIDTH:
        PUSH_INT(SAM_DISPLAY_WIDTH);
        break;
    case TRAP_GRAPHICS_DISPLAY_HEIGHT:
        PUSH_INT(SAM_DISPLAY_HEIGHT);
        break;
    case TRAP_GRAPHICS_CLEARSCREEN:
        {
            NVGcolor color;
            POP_COLOR(color);
            nvgBeginFrame(vg, SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT, (float)PIXEL_SIZE);
            nvgBeginPath(vg);
            nvgFillColor(vg, color);
            nvgRect(vg, 0, 0, SAM_DISPLAY_WIDTH * PIXEL_SIZE, SAM_DISPLAY_HEIGHT * PIXEL_SIZE);
            nvgFill(vg);
            nvgClosePath(vg);
            nvgEndFrame(vg);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_SETDOT:
        {
            sam_word_t x, y;
            NVGcolor color;
            POP_COLOR(color);
            POP_UINT(y);
            POP_UINT(x);
            nvgBeginFrame(vg, SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT, (float)PIXEL_SIZE);
            nvgBeginPath(vg);
            nvgFillColor(vg, color);
            nvgLineTo(vg, x * PIXEL_SIZE, y * PIXEL_SIZE);
            nvgRect(vg, x * PIXEL_SIZE, y * PIXEL_SIZE, PIXEL_SIZE, PIXEL_SIZE);
            nvgFill(vg);
            nvgClosePath(vg);
            nvgEndFrame(vg);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_DRAWLINE:
        {
            sam_word_t x1, y1, x2, y2;
            NVGcolor color;
            POP_COLOR(color);
            POP_UINT(y2);
            POP_UINT(x2);
            POP_UINT(y1);
            POP_UINT(x1);
            nvgBeginFrame(vg, SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT, (float)PIXEL_SIZE);
            nvgBeginPath(vg);
            nvgStrokeColor(vg, color);
            nvgMoveTo(vg, x1 * PIXEL_SIZE, y1 * PIXEL_SIZE);
            nvgLineTo(vg, x2 * PIXEL_SIZE, y2 * PIXEL_SIZE);
            nvgStrokeWidth(vg, PIXEL_SIZE);
            nvgStroke(vg);
            nvgClosePath(vg);
            nvgEndFrame(vg);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_DRAWRECT:
        {
            sam_word_t x, y, width, height;
            NVGcolor color;
            POP_COLOR(color);
            POP_UINT(height);
            POP_UINT(width);
            POP_UINT(y);
            POP_UINT(x);
            nvgBeginFrame(vg, SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT, (float)PIXEL_SIZE);
            nvgBeginPath(vg);
            nvgStrokeColor(vg, color);
            nvgRect(vg, x * PIXEL_SIZE, y * PIXEL_SIZE, width * PIXEL_SIZE, height * PIXEL_SIZE);
            nvgStrokeWidth(vg, PIXEL_SIZE);
            nvgStroke(vg);
            nvgClosePath(vg);
            nvgEndFrame(vg);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_DRAWROUNDRECT:
        {
            sam_word_t x, y, width, height, radius;
            NVGcolor color;
            POP_COLOR(color);
            POP_UINT(radius);
            POP_UINT(height);
            POP_UINT(width);
            POP_UINT(y);
            POP_UINT(x);
            nvgBeginFrame(vg, SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT, (float)PIXEL_SIZE);
            nvgBeginPath(vg);
            nvgStrokeColor(vg, color);
            nvgRoundedRect(vg, x * PIXEL_SIZE, y * PIXEL_SIZE, width * PIXEL_SIZE, height * PIXEL_SIZE, radius * PIXEL_SIZE);
            nvgStrokeWidth(vg, PIXEL_SIZE);
            nvgStroke(vg);
            nvgClosePath(vg);
            nvgEndFrame(vg);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_FILLRECT:
        {
            sam_word_t x, y, width, height;
            NVGcolor color;
            POP_COLOR(color);
            POP_UINT(height);
            POP_UINT(width);
            POP_UINT(y);
            POP_UINT(x);
            nvgBeginFrame(vg, SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT, (float)PIXEL_SIZE);
            nvgBeginPath(vg);
            nvgFillColor(vg, color);
            nvgRect(vg, x * PIXEL_SIZE, y * PIXEL_SIZE, width * PIXEL_SIZE, height * PIXEL_SIZE);
            nvgFill(vg);
            nvgClosePath(vg);
            nvgEndFrame(vg);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_DRAWCIRCLE:
        {
            sam_word_t xCenter, yCenter, radius;
            NVGcolor color;
            POP_COLOR(color);
            POP_UINT(radius);
            POP_UINT(yCenter);
            POP_UINT(xCenter);
            nvgBeginFrame(vg, SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT, (float)PIXEL_SIZE);
            nvgBeginPath(vg);
            nvgStrokeColor(vg, color);
            nvgCircle(vg, xCenter * PIXEL_SIZE, yCenter * PIXEL_SIZE, radius * PIXEL_SIZE);
            nvgStrokeWidth(vg, PIXEL_SIZE);
            nvgStroke(vg);
            nvgClosePath(vg);
            nvgEndFrame(vg);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_FILLCIRCLE:
        {
            sam_word_t xCenter, yCenter, radius;
            NVGcolor color;
            POP_COLOR(color);
            POP_UINT(radius);
            POP_UINT(yCenter);
            POP_UINT(xCenter);
            nvgBeginFrame(vg, SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT, (float)PIXEL_SIZE);
            nvgBeginPath(vg);
            nvgFillColor(vg, color);
            nvgCircle(vg, xCenter * PIXEL_SIZE, yCenter * PIXEL_SIZE, radius * PIXEL_SIZE);
            nvgFill(vg);
            nvgClosePath(vg);
            nvgEndFrame(vg);
            need_window = true;
        }
        break;
    case TRAP_GRAPHICS_DRAWBITMAP:
        {
            sam_word_t bitmap, x, y;
            NVGcolor color;
            POP_COLOR(color);
            POP_UINT(y);
            POP_UINT(x);
            POP_UINT(bitmap);
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

void sam_graphics_wait(void)
{
    while (sam_graphics_process_events() == 0) {
        SDL_Delay(sam_update_interval);
    }
}

char *sam_graphics_trap_name(sam_word_t function)
{
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
