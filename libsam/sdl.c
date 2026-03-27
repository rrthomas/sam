// SAM traps implemented with SDL: graphics, input.
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
#include "sam_sdl.h"

#include "private.h"
#include "run.h"
#include "traps_graphics.h"
#include "traps_input.h"

const unsigned sam_update_interval = 10; // milliseconds between screen updates

#define PIXEL_SIZE 2 // FIXME calculate pixel ratio
static SDL_Window *win;
static SDL_Surface *srf;
static Uint64 last_update_time;
static bool need_window = false;
static NVGcontext* vg;
static SDL_PixelFormat *fmt;
const unsigned bytes_per_pixel = 4;

enum font_handle {
    FONT_REGULAR,
    FONT_ITALIC,
    FONT_BOLD,
    FONT_BOLDITALIC,
    FONT_EMOJI,
    FONT_NUM_FONTS,
};

static int fonts[FONT_NUM_FONTS];

unsigned char font_regular[] = {
#embed "NotoSans-Regular.ttf"
};

unsigned char font_italic[] = {
#embed "NotoSans-Italic.ttf"
};

unsigned char font_bold[] = {
#embed "NotoSans-Bold.ttf"
};

unsigned char font_bolditalic[] = {
#embed "NotoSans-BoldItalic.ttf"
};

unsigned char font_emoji[] = {
/* #embed "NotoColorEmoji.ttf" */ // FIXME: stb_truetype fails to load this font.
#embed "NotoEmoji-Regular.ttf"
};

#define POP_COLOR(var)                                   \
    do {                                                 \
        sam_uword_t color_word;                          \
        POP_UINT(color_word);                            \
        var = (NVGcolor){.c = (unsigned int)color_word}; \
    } while (0)

static const Uint8 *keymap;
static int numkeys;

void sam_update_screen(void)
{
    SDL_ShowWindow(win);
    SDL_UpdateWindowSurface(win);
}

int sam_sdl_process_events(void)
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

int sam_sdl_window_used(void)
{
    return SDL_GetWindowFlags(win) & SDL_WINDOW_SHOWN;
}

// Code adapted from https://stackoverflow.com/questions/53033971/how-to-get-the-color-of-a-specific-pixel-from-sdl-surface
uint32_t sam_getpixel(int x, int y)
{
    Uint32 *p = (Uint32 *)((Uint8 *)srf->pixels + y * srf->pitch * PIXEL_SIZE + x * bytes_per_pixel * PIXEL_SIZE);
    return *p;
}

sam_word_t sam_sdl_init(void)
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

    fonts[FONT_REGULAR] = nvgCreateFontMem(vg, "Regular", font_regular, sizeof(font_regular), 0);
    fonts[FONT_ITALIC] = nvgCreateFontMem(vg, "Italic", font_italic, sizeof(font_italic), 0);
    fonts[FONT_BOLD] = nvgCreateFontMem(vg, "Bold", font_bold, sizeof(font_bold), 0);
    fonts[FONT_BOLDITALIC] = nvgCreateFontMem(vg, "BoldItalic", font_bolditalic, sizeof(font_bolditalic), 0);
    fonts[FONT_EMOJI] = nvgCreateFontMem(vg, "Emoji", font_emoji, sizeof(font_emoji), 0);
    nvgAddFallbackFontId(vg, fonts[FONT_REGULAR], fonts[FONT_EMOJI]);
    // Use Emoji font as fallback for all the rest.
    for (int i = 0; i < FONT_NUM_FONTS; i++)
        nvgAddFallbackFontId(vg, fonts[i], fonts[FONT_EMOJI]);

    keymap = SDL_GetKeyboardState(&numkeys);

    last_update_time = 0;

    return SAM_ERROR_OK;
}

void sam_sdl_finish(void)
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
    case TRAP_GRAPHICS_FONT_REGULAR:
        PUSH_INT(FONT_REGULAR);
        break;
    case TRAP_GRAPHICS_FONT_ITALIC:
        PUSH_INT(FONT_ITALIC);
        break;
    case TRAP_GRAPHICS_FONT_BOLD:
        PUSH_INT(FONT_BOLD);
        break;
    case TRAP_GRAPHICS_FONT_BOLDITALIC:
        PUSH_INT(FONT_BOLDITALIC);
        break;
    case TRAP_GRAPHICS_FONT_EMOJI:
        PUSH_INT(FONT_EMOJI);
        break;
    case TRAP_GRAPHICS_TEXT:
        {
            sam_uword_t font;
            POP_UINT(font);
            if (font > FONT_NUM_FONTS)
                HALT(SAM_ERROR_WRONG_TYPE); // FIXME: better error
            NVGcolor color;
            POP_COLOR(color);
            sam_word_t x, y;
            POP_UINT(y);
            POP_UINT(x);
            sam_blob_t *blob;
            POP_REF(blob);
            sam_string_t *str;
            EXTRACT_BLOB(blob, SAM_BLOB_STRING, sam_string_t, str);

            // FIXME: make the following parameters or state
            nvgBeginFrame(vg, SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT, (float)PIXEL_SIZE);
            nvgFontFaceId(vg, fonts[font]);
            nvgFontSize(vg, 16.0 * PIXEL_SIZE);
            nvgFillColor(vg, color);
            nvgBeginPath(vg);
            nvgText(vg, x, y, str->str, str->str + str->len);
            nvgClosePath(vg);
            nvgEndFrame(vg);
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

void sam_sdl_wait(void)
{
    while (sam_sdl_process_events() == 0) {
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
    case TRAP_GRAPHICS_FONT_REGULAR:
        return "FONT_REGULAR";
    case TRAP_GRAPHICS_FONT_ITALIC:
        return "FONT_ITALIC";
    case TRAP_GRAPHICS_FONT_BOLD:
        return "FONT_BOLD";
    case TRAP_GRAPHICS_FONT_BOLDITALIC:
        return "FONT_BOLDITALIC";
    case TRAP_GRAPHICS_FONT_EMOJI:
        return "FONT_EMOJI";
    case TRAP_GRAPHICS_TEXT:
        return "TEXT";
    default:
        return NULL;
    }
}

sam_word_t sam_input_trap(sam_state_t *state, sam_uword_t function)
{
#define s ((sam_stack_t *)state->s0->data)
    int error = SAM_ERROR_OK;

    switch (function) {
    case TRAP_INPUT_KEYPRESSED:
        {
            sam_word_t key;
            POP_INT(key);
            if (key >= numkeys)
                HALT(SAM_ERROR_WRONG_TYPE);
            SDL_PumpEvents();
            PUSH_INT(keymap[key]);
            need_window = true;
        }
        break;
    case TRAP_INPUT_KEY_A:
        PUSH_INT(SDL_SCANCODE_A);
        break;
    case TRAP_INPUT_KEY_B:
        PUSH_INT(SDL_SCANCODE_B);
        break;
    case TRAP_INPUT_KEY_C:
        PUSH_INT(SDL_SCANCODE_C);
        break;
    case TRAP_INPUT_KEY_D:
        PUSH_INT(SDL_SCANCODE_D);
        break;
    case TRAP_INPUT_KEY_E:
        PUSH_INT(SDL_SCANCODE_E);
        break;
    case TRAP_INPUT_KEY_F:
        PUSH_INT(SDL_SCANCODE_F);
        break;
    case TRAP_INPUT_KEY_G:
        PUSH_INT(SDL_SCANCODE_G);
        break;
    case TRAP_INPUT_KEY_H:
        PUSH_INT(SDL_SCANCODE_H);
        break;
    case TRAP_INPUT_KEY_I:
        PUSH_INT(SDL_SCANCODE_I);
        break;
    case TRAP_INPUT_KEY_J:
        PUSH_INT(SDL_SCANCODE_J);
        break;
    case TRAP_INPUT_KEY_K:
        PUSH_INT(SDL_SCANCODE_K);
        break;
    case TRAP_INPUT_KEY_L:
        PUSH_INT(SDL_SCANCODE_L);
        break;
    case TRAP_INPUT_KEY_M:
        PUSH_INT(SDL_SCANCODE_M);
        break;
    case TRAP_INPUT_KEY_N:
        PUSH_INT(SDL_SCANCODE_N);
        break;
    case TRAP_INPUT_KEY_O:
        PUSH_INT(SDL_SCANCODE_O);
        break;
    case TRAP_INPUT_KEY_P:
        PUSH_INT(SDL_SCANCODE_P);
        break;
    case TRAP_INPUT_KEY_Q:
        PUSH_INT(SDL_SCANCODE_Q);
        break;
    case TRAP_INPUT_KEY_R:
        PUSH_INT(SDL_SCANCODE_R);
        break;
    case TRAP_INPUT_KEY_S:
        PUSH_INT(SDL_SCANCODE_S);
        break;
    case TRAP_INPUT_KEY_T:
        PUSH_INT(SDL_SCANCODE_T);
        break;
    case TRAP_INPUT_KEY_U:
        PUSH_INT(SDL_SCANCODE_U);
        break;
    case TRAP_INPUT_KEY_V:
        PUSH_INT(SDL_SCANCODE_V);
        break;
    case TRAP_INPUT_KEY_W:
        PUSH_INT(SDL_SCANCODE_W);
        break;
    case TRAP_INPUT_KEY_X:
        PUSH_INT(SDL_SCANCODE_X);
        break;
    case TRAP_INPUT_KEY_Y:
        PUSH_INT(SDL_SCANCODE_Y);
        break;
    case TRAP_INPUT_KEY_Z:
        PUSH_INT(SDL_SCANCODE_Z);
        break;
    case TRAP_INPUT_KEY_1:
        PUSH_INT(SDL_SCANCODE_1);
        break;
    case TRAP_INPUT_KEY_2:
        PUSH_INT(SDL_SCANCODE_2);
        break;
    case TRAP_INPUT_KEY_3:
        PUSH_INT(SDL_SCANCODE_3);
        break;
    case TRAP_INPUT_KEY_4:
        PUSH_INT(SDL_SCANCODE_4);
        break;
    case TRAP_INPUT_KEY_5:
        PUSH_INT(SDL_SCANCODE_5);
        break;
    case TRAP_INPUT_KEY_6:
        PUSH_INT(SDL_SCANCODE_6);
        break;
    case TRAP_INPUT_KEY_7:
        PUSH_INT(SDL_SCANCODE_7);
        break;
    case TRAP_INPUT_KEY_8:
        PUSH_INT(SDL_SCANCODE_8);
        break;
    case TRAP_INPUT_KEY_9:
        PUSH_INT(SDL_SCANCODE_9);
        break;
    case TRAP_INPUT_KEY_0:
        PUSH_INT(SDL_SCANCODE_0);
        break;
    case TRAP_INPUT_KEY_RETURN:
        PUSH_INT(SDL_SCANCODE_RETURN);
        break;
    case TRAP_INPUT_KEY_ESCAPE:
        PUSH_INT(SDL_SCANCODE_ESCAPE);
        break;
    case TRAP_INPUT_KEY_BACKSPACE:
        PUSH_INT(SDL_SCANCODE_BACKSPACE);
        break;
    case TRAP_INPUT_KEY_TAB:
        PUSH_INT(SDL_SCANCODE_TAB);
        break;
    case TRAP_INPUT_KEY_SPACE:
        PUSH_INT(SDL_SCANCODE_SPACE);
        break;
    case TRAP_INPUT_KEY_MINUS:
        PUSH_INT(SDL_SCANCODE_MINUS);
        break;
    case TRAP_INPUT_KEY_EQUALS:
        PUSH_INT(SDL_SCANCODE_EQUALS);
        break;
    case TRAP_INPUT_KEY_LEFTBRACKET:
        PUSH_INT(SDL_SCANCODE_LEFTBRACKET);
        break;
    case TRAP_INPUT_KEY_RIGHTBRACKET:
        PUSH_INT(SDL_SCANCODE_RIGHTBRACKET);
        break;
    case TRAP_INPUT_KEY_BACKSLASH:
        PUSH_INT(SDL_SCANCODE_BACKSLASH);
        break;
    case TRAP_INPUT_KEY_SEMICOLON:
        PUSH_INT(SDL_SCANCODE_SEMICOLON);
        break;
    case TRAP_INPUT_KEY_APOSTROPHE:
        PUSH_INT(SDL_SCANCODE_APOSTROPHE);
        break;
    case TRAP_INPUT_KEY_GRAVE:
        PUSH_INT(SDL_SCANCODE_GRAVE);
        break;
    case TRAP_INPUT_KEY_COMMA:
        PUSH_INT(SDL_SCANCODE_COMMA);
        break;
    case TRAP_INPUT_KEY_PERIOD:
        PUSH_INT(SDL_SCANCODE_PERIOD);
        break;
    case TRAP_INPUT_KEY_SLASH:
        PUSH_INT(SDL_SCANCODE_SLASH);
        break;
    case TRAP_INPUT_KEY_CAPSLOCK:
        PUSH_INT(SDL_SCANCODE_CAPSLOCK);
        break;
    case TRAP_INPUT_KEY_F1:
        PUSH_INT(SDL_SCANCODE_F1);
        break;
    case TRAP_INPUT_KEY_F2:
        PUSH_INT(SDL_SCANCODE_F2);
        break;
    case TRAP_INPUT_KEY_F3:
        PUSH_INT(SDL_SCANCODE_F3);
        break;
    case TRAP_INPUT_KEY_F4:
        PUSH_INT(SDL_SCANCODE_F4);
        break;
    case TRAP_INPUT_KEY_F5:
        PUSH_INT(SDL_SCANCODE_F5);
        break;
    case TRAP_INPUT_KEY_F6:
        PUSH_INT(SDL_SCANCODE_F6);
        break;
    case TRAP_INPUT_KEY_F7:
        PUSH_INT(SDL_SCANCODE_F7);
        break;
    case TRAP_INPUT_KEY_F8:
        PUSH_INT(SDL_SCANCODE_F8);
        break;
    case TRAP_INPUT_KEY_F9:
        PUSH_INT(SDL_SCANCODE_F9);
        break;
    case TRAP_INPUT_KEY_F10:
        PUSH_INT(SDL_SCANCODE_F10);
        break;
    case TRAP_INPUT_KEY_F11:
        PUSH_INT(SDL_SCANCODE_F11);
        break;
    case TRAP_INPUT_KEY_F12:
        PUSH_INT(SDL_SCANCODE_F12);
        break;
    case TRAP_INPUT_KEY_PRINTSCREEN:
        PUSH_INT(SDL_SCANCODE_PRINTSCREEN);
        break;
    case TRAP_INPUT_KEY_SCROLLLOCK:
        PUSH_INT(SDL_SCANCODE_SCROLLLOCK);
        break;
    case TRAP_INPUT_KEY_PAUSE:
        PUSH_INT(SDL_SCANCODE_PAUSE);
        break;
    case TRAP_INPUT_KEY_INSERT:
        PUSH_INT(SDL_SCANCODE_INSERT);
        break;
    case TRAP_INPUT_KEY_HOME:
        PUSH_INT(SDL_SCANCODE_HOME);
        break;
    case TRAP_INPUT_KEY_PAGEUP:
        PUSH_INT(SDL_SCANCODE_PAGEUP);
        break;
    case TRAP_INPUT_KEY_DELETE:
        PUSH_INT(SDL_SCANCODE_DELETE);
        break;
    case TRAP_INPUT_KEY_END:
        PUSH_INT(SDL_SCANCODE_END);
        break;
    case TRAP_INPUT_KEY_PAGEDOWN:
        PUSH_INT(SDL_SCANCODE_PAGEDOWN);
        break;
    case TRAP_INPUT_KEY_RIGHT:
        PUSH_INT(SDL_SCANCODE_RIGHT);
        break;
    case TRAP_INPUT_KEY_LEFT:
        PUSH_INT(SDL_SCANCODE_LEFT);
        break;
    case TRAP_INPUT_KEY_DOWN:
        PUSH_INT(SDL_SCANCODE_DOWN);
        break;
    case TRAP_INPUT_KEY_UP:
        PUSH_INT(SDL_SCANCODE_UP);
        break;
    case TRAP_INPUT_KEY_NUMLOCKCLEAR:
        PUSH_INT(SDL_SCANCODE_NUMLOCKCLEAR);
        break;
    case TRAP_INPUT_KEY_KP_DIVIDE:
        PUSH_INT(SDL_SCANCODE_KP_DIVIDE);
        break;
    case TRAP_INPUT_KEY_KP_MULTIPLY:
        PUSH_INT(SDL_SCANCODE_KP_MULTIPLY);
        break;
    case TRAP_INPUT_KEY_KP_MINUS:
        PUSH_INT(SDL_SCANCODE_KP_MINUS);
        break;
    case TRAP_INPUT_KEY_KP_PLUS:
        PUSH_INT(SDL_SCANCODE_KP_PLUS);
        break;
    case TRAP_INPUT_KEY_KP_ENTER:
        PUSH_INT(SDL_SCANCODE_KP_ENTER);
        break;
    case TRAP_INPUT_KEY_KP_1:
        PUSH_INT(SDL_SCANCODE_KP_1);
        break;
    case TRAP_INPUT_KEY_KP_2:
        PUSH_INT(SDL_SCANCODE_KP_2);
        break;
    case TRAP_INPUT_KEY_KP_3:
        PUSH_INT(SDL_SCANCODE_KP_3);
        break;
    case TRAP_INPUT_KEY_KP_4:
        PUSH_INT(SDL_SCANCODE_KP_4);
        break;
    case TRAP_INPUT_KEY_KP_5:
        PUSH_INT(SDL_SCANCODE_KP_5);
        break;
    case TRAP_INPUT_KEY_KP_6:
        PUSH_INT(SDL_SCANCODE_KP_6);
        break;
    case TRAP_INPUT_KEY_KP_7:
        PUSH_INT(SDL_SCANCODE_KP_7);
        break;
    case TRAP_INPUT_KEY_KP_8:
        PUSH_INT(SDL_SCANCODE_KP_8);
        break;
    case TRAP_INPUT_KEY_KP_9:
        PUSH_INT(SDL_SCANCODE_KP_9);
        break;
    case TRAP_INPUT_KEY_KP_0:
        PUSH_INT(SDL_SCANCODE_KP_0);
        break;
    case TRAP_INPUT_KEY_KP_PERIOD:
        PUSH_INT(SDL_SCANCODE_KP_PERIOD);
        break;
    case TRAP_INPUT_KEY_NONUSBACKSLASH:
        PUSH_INT(SDL_SCANCODE_NONUSBACKSLASH);
        break;
    case TRAP_INPUT_KEY_APPLICATION:
        PUSH_INT(SDL_SCANCODE_APPLICATION);
        break;
    case TRAP_INPUT_KEY_POWER:
        PUSH_INT(SDL_SCANCODE_POWER);
        break;
    case TRAP_INPUT_KEY_KP_EQUALS:
        PUSH_INT(SDL_SCANCODE_KP_EQUALS);
        break;
    case TRAP_INPUT_KEY_F13:
        PUSH_INT(SDL_SCANCODE_F13);
        break;
    case TRAP_INPUT_KEY_F14:
        PUSH_INT(SDL_SCANCODE_F14);
        break;
    case TRAP_INPUT_KEY_F15:
        PUSH_INT(SDL_SCANCODE_F15);
        break;
    case TRAP_INPUT_KEY_F16:
        PUSH_INT(SDL_SCANCODE_F16);
        break;
    case TRAP_INPUT_KEY_F17:
        PUSH_INT(SDL_SCANCODE_F17);
        break;
    case TRAP_INPUT_KEY_F18:
        PUSH_INT(SDL_SCANCODE_F18);
        break;
    case TRAP_INPUT_KEY_F19:
        PUSH_INT(SDL_SCANCODE_F19);
        break;
    case TRAP_INPUT_KEY_F20:
        PUSH_INT(SDL_SCANCODE_F20);
        break;
    case TRAP_INPUT_KEY_F21:
        PUSH_INT(SDL_SCANCODE_F21);
        break;
    case TRAP_INPUT_KEY_F22:
        PUSH_INT(SDL_SCANCODE_F22);
        break;
    case TRAP_INPUT_KEY_F23:
        PUSH_INT(SDL_SCANCODE_F23);
        break;
    case TRAP_INPUT_KEY_F24:
        PUSH_INT(SDL_SCANCODE_F24);
        break;
    case TRAP_INPUT_KEY_EXECUTE:
        PUSH_INT(SDL_SCANCODE_EXECUTE);
        break;
    case TRAP_INPUT_KEY_HELP:
        PUSH_INT(SDL_SCANCODE_HELP);
        break;
    case TRAP_INPUT_KEY_MENU:
        PUSH_INT(SDL_SCANCODE_MENU);
        break;
    case TRAP_INPUT_KEY_SELECT:
        PUSH_INT(SDL_SCANCODE_SELECT);
        break;
    case TRAP_INPUT_KEY_STOP:
        PUSH_INT(SDL_SCANCODE_STOP);
        break;
    case TRAP_INPUT_KEY_AGAIN:
        PUSH_INT(SDL_SCANCODE_AGAIN);
        break;
    case TRAP_INPUT_KEY_UNDO:
        PUSH_INT(SDL_SCANCODE_UNDO);
        break;
    case TRAP_INPUT_KEY_CUT:
        PUSH_INT(SDL_SCANCODE_CUT);
        break;
    case TRAP_INPUT_KEY_COPY:
        PUSH_INT(SDL_SCANCODE_COPY);
        break;
    case TRAP_INPUT_KEY_PASTE:
        PUSH_INT(SDL_SCANCODE_PASTE);
        break;
    case TRAP_INPUT_KEY_FIND:
        PUSH_INT(SDL_SCANCODE_FIND);
        break;
    case TRAP_INPUT_KEY_MUTE:
        PUSH_INT(SDL_SCANCODE_MUTE);
        break;
    case TRAP_INPUT_KEY_VOLUMEUP:
        PUSH_INT(SDL_SCANCODE_VOLUMEUP);
        break;
    case TRAP_INPUT_KEY_VOLUMEDOWN:
        PUSH_INT(SDL_SCANCODE_VOLUMEDOWN);
        break;
    case TRAP_INPUT_KEY_KP_COMMA:
        PUSH_INT(SDL_SCANCODE_KP_COMMA);
        break;
    case TRAP_INPUT_KEY_KP_EQUALSAS400:
        PUSH_INT(SDL_SCANCODE_KP_EQUALSAS400);
        break;
    case TRAP_INPUT_KEY_INTERNATIONAL1:
        PUSH_INT(SDL_SCANCODE_INTERNATIONAL1);
        break;
    case TRAP_INPUT_KEY_INTERNATIONAL2:
        PUSH_INT(SDL_SCANCODE_INTERNATIONAL2);
        break;
    case TRAP_INPUT_KEY_INTERNATIONAL3:
        PUSH_INT(SDL_SCANCODE_INTERNATIONAL3);
        break;
    case TRAP_INPUT_KEY_INTERNATIONAL4:
        PUSH_INT(SDL_SCANCODE_INTERNATIONAL4);
        break;
    case TRAP_INPUT_KEY_INTERNATIONAL5:
        PUSH_INT(SDL_SCANCODE_INTERNATIONAL5);
        break;
    case TRAP_INPUT_KEY_INTERNATIONAL6:
        PUSH_INT(SDL_SCANCODE_INTERNATIONAL6);
        break;
    case TRAP_INPUT_KEY_INTERNATIONAL7:
        PUSH_INT(SDL_SCANCODE_INTERNATIONAL7);
        break;
    case TRAP_INPUT_KEY_INTERNATIONAL8:
        PUSH_INT(SDL_SCANCODE_INTERNATIONAL8);
        break;
    case TRAP_INPUT_KEY_INTERNATIONAL9:
        PUSH_INT(SDL_SCANCODE_INTERNATIONAL9);
        break;
    case TRAP_INPUT_KEY_LANG1:
        PUSH_INT(SDL_SCANCODE_LANG1);
        break;
    case TRAP_INPUT_KEY_LANG2:
        PUSH_INT(SDL_SCANCODE_LANG2);
        break;
    case TRAP_INPUT_KEY_LANG3:
        PUSH_INT(SDL_SCANCODE_LANG3);
        break;
    case TRAP_INPUT_KEY_LANG4:
        PUSH_INT(SDL_SCANCODE_LANG4);
        break;
    case TRAP_INPUT_KEY_LANG5:
        PUSH_INT(SDL_SCANCODE_LANG5);
        break;
    case TRAP_INPUT_KEY_LANG6:
        PUSH_INT(SDL_SCANCODE_LANG6);
        break;
    case TRAP_INPUT_KEY_LANG7:
        PUSH_INT(SDL_SCANCODE_LANG7);
        break;
    case TRAP_INPUT_KEY_LANG8:
        PUSH_INT(SDL_SCANCODE_LANG8);
        break;
    case TRAP_INPUT_KEY_LANG9:
        PUSH_INT(SDL_SCANCODE_LANG9);
        break;
    case TRAP_INPUT_KEY_ALTERASE:
        PUSH_INT(SDL_SCANCODE_ALTERASE);
        break;
    case TRAP_INPUT_KEY_SYSREQ:
        PUSH_INT(SDL_SCANCODE_SYSREQ);
        break;
    case TRAP_INPUT_KEY_CANCEL:
        PUSH_INT(SDL_SCANCODE_CANCEL);
        break;
    case TRAP_INPUT_KEY_CLEAR:
        PUSH_INT(SDL_SCANCODE_CLEAR);
        break;
    case TRAP_INPUT_KEY_PRIOR:
        PUSH_INT(SDL_SCANCODE_PRIOR);
        break;
    case TRAP_INPUT_KEY_RETURN2:
        PUSH_INT(SDL_SCANCODE_RETURN2);
        break;
    case TRAP_INPUT_KEY_SEPARATOR:
        PUSH_INT(SDL_SCANCODE_SEPARATOR);
        break;
    case TRAP_INPUT_KEY_OUT:
        PUSH_INT(SDL_SCANCODE_OUT);
        break;
    case TRAP_INPUT_KEY_OPER:
        PUSH_INT(SDL_SCANCODE_OPER);
        break;
    case TRAP_INPUT_KEY_CLEARAGAIN:
        PUSH_INT(SDL_SCANCODE_CLEARAGAIN);
        break;
    case TRAP_INPUT_KEY_CRSEL:
        PUSH_INT(SDL_SCANCODE_CRSEL);
        break;
    case TRAP_INPUT_KEY_EXSEL:
        PUSH_INT(SDL_SCANCODE_EXSEL);
        break;
    case TRAP_INPUT_KEY_KP_00:
        PUSH_INT(SDL_SCANCODE_KP_00);
        break;
    case TRAP_INPUT_KEY_KP_000:
        PUSH_INT(SDL_SCANCODE_KP_000);
        break;
    case TRAP_INPUT_KEY_THOUSANDSSEPARATOR:
        PUSH_INT(SDL_SCANCODE_THOUSANDSSEPARATOR);
        break;
    case TRAP_INPUT_KEY_DECIMALSEPARATOR:
        PUSH_INT(SDL_SCANCODE_DECIMALSEPARATOR);
        break;
    case TRAP_INPUT_KEY_CURRENCYUNIT:
        PUSH_INT(SDL_SCANCODE_CURRENCYUNIT);
        break;
    case TRAP_INPUT_KEY_CURRENCYSUBUNIT:
        PUSH_INT(SDL_SCANCODE_CURRENCYSUBUNIT);
        break;
    case TRAP_INPUT_KEY_KP_LEFTPAREN:
        PUSH_INT(SDL_SCANCODE_KP_LEFTPAREN);
        break;
    case TRAP_INPUT_KEY_KP_RIGHTPAREN:
        PUSH_INT(SDL_SCANCODE_KP_RIGHTPAREN);
        break;
    case TRAP_INPUT_KEY_KP_LEFTBRACE:
        PUSH_INT(SDL_SCANCODE_KP_LEFTBRACE);
        break;
    case TRAP_INPUT_KEY_KP_RIGHTBRACE:
        PUSH_INT(SDL_SCANCODE_KP_RIGHTBRACE);
        break;
    case TRAP_INPUT_KEY_KP_TAB:
        PUSH_INT(SDL_SCANCODE_KP_TAB);
        break;
    case TRAP_INPUT_KEY_KP_BACKSPACE:
        PUSH_INT(SDL_SCANCODE_KP_BACKSPACE);
        break;
    case TRAP_INPUT_KEY_KP_A:
        PUSH_INT(SDL_SCANCODE_KP_A);
        break;
    case TRAP_INPUT_KEY_KP_B:
        PUSH_INT(SDL_SCANCODE_KP_B);
        break;
    case TRAP_INPUT_KEY_KP_C:
        PUSH_INT(SDL_SCANCODE_KP_C);
        break;
    case TRAP_INPUT_KEY_KP_D:
        PUSH_INT(SDL_SCANCODE_KP_D);
        break;
    case TRAP_INPUT_KEY_KP_E:
        PUSH_INT(SDL_SCANCODE_KP_E);
        break;
    case TRAP_INPUT_KEY_KP_F:
        PUSH_INT(SDL_SCANCODE_KP_F);
        break;
    case TRAP_INPUT_KEY_KP_XOR:
        PUSH_INT(SDL_SCANCODE_KP_XOR);
        break;
    case TRAP_INPUT_KEY_KP_POWER:
        PUSH_INT(SDL_SCANCODE_KP_POWER);
        break;
    case TRAP_INPUT_KEY_KP_PERCENT:
        PUSH_INT(SDL_SCANCODE_KP_PERCENT);
        break;
    case TRAP_INPUT_KEY_KP_LESS:
        PUSH_INT(SDL_SCANCODE_KP_LESS);
        break;
    case TRAP_INPUT_KEY_KP_GREATER:
        PUSH_INT(SDL_SCANCODE_KP_GREATER);
        break;
    case TRAP_INPUT_KEY_KP_AMPERSAND:
        PUSH_INT(SDL_SCANCODE_KP_AMPERSAND);
        break;
    case TRAP_INPUT_KEY_KP_DBLAMPERSAND:
        PUSH_INT(SDL_SCANCODE_KP_DBLAMPERSAND);
        break;
    case TRAP_INPUT_KEY_KP_VERTICALBAR:
        PUSH_INT(SDL_SCANCODE_KP_VERTICALBAR);
        break;
    case TRAP_INPUT_KEY_KP_DBLVERTICALBAR:
        PUSH_INT(SDL_SCANCODE_KP_DBLVERTICALBAR);
        break;
    case TRAP_INPUT_KEY_KP_COLON:
        PUSH_INT(SDL_SCANCODE_KP_COLON);
        break;
    case TRAP_INPUT_KEY_KP_HASH:
        PUSH_INT(SDL_SCANCODE_KP_HASH);
        break;
    case TRAP_INPUT_KEY_KP_SPACE:
        PUSH_INT(SDL_SCANCODE_KP_SPACE);
        break;
    case TRAP_INPUT_KEY_KP_AT:
        PUSH_INT(SDL_SCANCODE_KP_AT);
        break;
    case TRAP_INPUT_KEY_KP_EXCLAM:
        PUSH_INT(SDL_SCANCODE_KP_EXCLAM);
        break;
    case TRAP_INPUT_KEY_KP_MEMSTORE:
        PUSH_INT(SDL_SCANCODE_KP_MEMSTORE);
        break;
    case TRAP_INPUT_KEY_KP_MEMRECALL:
        PUSH_INT(SDL_SCANCODE_KP_MEMRECALL);
        break;
    case TRAP_INPUT_KEY_KP_MEMCLEAR:
        PUSH_INT(SDL_SCANCODE_KP_MEMCLEAR);
        break;
    case TRAP_INPUT_KEY_KP_MEMADD:
        PUSH_INT(SDL_SCANCODE_KP_MEMADD);
        break;
    case TRAP_INPUT_KEY_KP_MEMSUBTRACT:
        PUSH_INT(SDL_SCANCODE_KP_MEMSUBTRACT);
        break;
    case TRAP_INPUT_KEY_KP_MEMMULTIPLY:
        PUSH_INT(SDL_SCANCODE_KP_MEMMULTIPLY);
        break;
    case TRAP_INPUT_KEY_KP_MEMDIVIDE:
        PUSH_INT(SDL_SCANCODE_KP_MEMDIVIDE);
        break;
    case TRAP_INPUT_KEY_KP_PLUSMINUS:
        PUSH_INT(SDL_SCANCODE_KP_PLUSMINUS);
        break;
    case TRAP_INPUT_KEY_KP_CLEAR:
        PUSH_INT(SDL_SCANCODE_KP_CLEAR);
        break;
    case TRAP_INPUT_KEY_KP_CLEARENTRY:
        PUSH_INT(SDL_SCANCODE_KP_CLEARENTRY);
        break;
    case TRAP_INPUT_KEY_KP_BINARY:
        PUSH_INT(SDL_SCANCODE_KP_BINARY);
        break;
    case TRAP_INPUT_KEY_KP_OCTAL:
        PUSH_INT(SDL_SCANCODE_KP_OCTAL);
        break;
    case TRAP_INPUT_KEY_KP_DECIMAL:
        PUSH_INT(SDL_SCANCODE_KP_DECIMAL);
        break;
    case TRAP_INPUT_KEY_KP_HEXADECIMAL:
        PUSH_INT(SDL_SCANCODE_KP_HEXADECIMAL);
        break;
    case TRAP_INPUT_KEY_LCTRL:
        PUSH_INT(SDL_SCANCODE_LCTRL);
        break;
    case TRAP_INPUT_KEY_LSHIFT:
        PUSH_INT(SDL_SCANCODE_LSHIFT);
        break;
    case TRAP_INPUT_KEY_LALT:
        PUSH_INT(SDL_SCANCODE_LALT);
        break;
    case TRAP_INPUT_KEY_LGUI:
        PUSH_INT(SDL_SCANCODE_LGUI);
        break;
    case TRAP_INPUT_KEY_RCTRL:
        PUSH_INT(SDL_SCANCODE_RCTRL);
        break;
    case TRAP_INPUT_KEY_RSHIFT:
        PUSH_INT(SDL_SCANCODE_RSHIFT);
        break;
    case TRAP_INPUT_KEY_RALT:
        PUSH_INT(SDL_SCANCODE_RALT);
        break;
    case TRAP_INPUT_KEY_RGUI:
        PUSH_INT(SDL_SCANCODE_RGUI);
        break;
    case TRAP_INPUT_KEY_MODE:
        PUSH_INT(SDL_SCANCODE_MODE);
        break;
    case TRAP_INPUT_KEY_AUDIONEXT:
        PUSH_INT(SDL_SCANCODE_AUDIONEXT);
        break;
    case TRAP_INPUT_KEY_AUDIOPREV:
        PUSH_INT(SDL_SCANCODE_AUDIOPREV);
        break;
    case TRAP_INPUT_KEY_AUDIOSTOP:
        PUSH_INT(SDL_SCANCODE_AUDIOSTOP);
        break;
    case TRAP_INPUT_KEY_AUDIOPLAY:
        PUSH_INT(SDL_SCANCODE_AUDIOPLAY);
        break;
    case TRAP_INPUT_KEY_AUDIOMUTE:
        PUSH_INT(SDL_SCANCODE_AUDIOMUTE);
        break;
    case TRAP_INPUT_KEY_MEDIASELECT:
        PUSH_INT(SDL_SCANCODE_MEDIASELECT);
        break;
    case TRAP_INPUT_KEY_WWW:
        PUSH_INT(SDL_SCANCODE_WWW);
        break;
    case TRAP_INPUT_KEY_MAIL:
        PUSH_INT(SDL_SCANCODE_MAIL);
        break;
    case TRAP_INPUT_KEY_CALCULATOR:
        PUSH_INT(SDL_SCANCODE_CALCULATOR);
        break;
    case TRAP_INPUT_KEY_COMPUTER:
        PUSH_INT(SDL_SCANCODE_COMPUTER);
        break;
    case TRAP_INPUT_KEY_AC_SEARCH:
        PUSH_INT(SDL_SCANCODE_AC_SEARCH);
        break;
    case TRAP_INPUT_KEY_AC_HOME:
        PUSH_INT(SDL_SCANCODE_AC_HOME);
        break;
    case TRAP_INPUT_KEY_AC_BACK:
        PUSH_INT(SDL_SCANCODE_AC_BACK);
        break;
    case TRAP_INPUT_KEY_AC_FORWARD:
        PUSH_INT(SDL_SCANCODE_AC_FORWARD);
        break;
    case TRAP_INPUT_KEY_AC_STOP:
        PUSH_INT(SDL_SCANCODE_AC_STOP);
        break;
    case TRAP_INPUT_KEY_AC_REFRESH:
        PUSH_INT(SDL_SCANCODE_AC_REFRESH);
        break;
    case TRAP_INPUT_KEY_AC_BOOKMARKS:
        PUSH_INT(SDL_SCANCODE_AC_BOOKMARKS);
        break;
    case TRAP_INPUT_KEY_BRIGHTNESSDOWN:
        PUSH_INT(SDL_SCANCODE_BRIGHTNESSDOWN);
        break;
    case TRAP_INPUT_KEY_BRIGHTNESSUP:
        PUSH_INT(SDL_SCANCODE_BRIGHTNESSUP);
        break;
    case TRAP_INPUT_KEY_DISPLAYSWITCH:
        PUSH_INT(SDL_SCANCODE_DISPLAYSWITCH);
        break;
    case TRAP_INPUT_KEY_KBDILLUMTOGGLE:
        PUSH_INT(SDL_SCANCODE_KBDILLUMTOGGLE);
        break;
    case TRAP_INPUT_KEY_KBDILLUMDOWN:
        PUSH_INT(SDL_SCANCODE_KBDILLUMDOWN);
        break;
    case TRAP_INPUT_KEY_KBDILLUMUP:
        PUSH_INT(SDL_SCANCODE_KBDILLUMUP);
        break;
    case TRAP_INPUT_KEY_EJECT:
        PUSH_INT(SDL_SCANCODE_EJECT);
        break;
    case TRAP_INPUT_KEY_SLEEP:
        PUSH_INT(SDL_SCANCODE_SLEEP);
        break;
    case TRAP_INPUT_KEY_APP1:
        PUSH_INT(SDL_SCANCODE_APP1);
        break;
    case TRAP_INPUT_KEY_APP2:
        PUSH_INT(SDL_SCANCODE_APP2);
        break;
    case TRAP_INPUT_KEY_AUDIOREWIND:
        PUSH_INT(SDL_SCANCODE_AUDIOREWIND);
        break;
    case TRAP_INPUT_KEY_AUDIOFASTFORWARD:
        PUSH_INT(SDL_SCANCODE_AUDIOFASTFORWARD);
        break;
    case TRAP_INPUT_KEY_SOFTLEFT:
        PUSH_INT(SDL_SCANCODE_SOFTLEFT);
        break;
    case TRAP_INPUT_KEY_SOFTRIGHT:
        PUSH_INT(SDL_SCANCODE_SOFTRIGHT);
        break;
    case TRAP_INPUT_KEY_CALL:
        PUSH_INT(SDL_SCANCODE_CALL);
        break;
    case TRAP_INPUT_KEY_ENDCALL:
        PUSH_INT(SDL_SCANCODE_ENDCALL);
        break;
    default:
        error = SAM_ERROR_INVALID_TRAP;
        break;
    }

 error:
    return error;
}

char *sam_input_trap_name(sam_word_t function)
{
    switch (function) {
    case TRAP_INPUT_KEYPRESSED:
      return "KEYPRESSED";
    case TRAP_INPUT_KEY_A:
      return "KEY_A";
    case TRAP_INPUT_KEY_B:
      return "KEY_B";
    case TRAP_INPUT_KEY_C:
      return "KEY_C";
    case TRAP_INPUT_KEY_D:
      return "KEY_D";
    case TRAP_INPUT_KEY_E:
      return "KEY_E";
    case TRAP_INPUT_KEY_F:
      return "KEY_F";
    case TRAP_INPUT_KEY_G:
      return "KEY_G";
    case TRAP_INPUT_KEY_H:
      return "KEY_H";
    case TRAP_INPUT_KEY_I:
      return "KEY_I";
    case TRAP_INPUT_KEY_J:
      return "KEY_J";
    case TRAP_INPUT_KEY_K:
      return "KEY_K";
    case TRAP_INPUT_KEY_L:
      return "KEY_L";
    case TRAP_INPUT_KEY_M:
      return "KEY_M";
    case TRAP_INPUT_KEY_N:
      return "KEY_N";
    case TRAP_INPUT_KEY_O:
      return "KEY_O";
    case TRAP_INPUT_KEY_P:
      return "KEY_P";
    case TRAP_INPUT_KEY_Q:
      return "KEY_Q";
    case TRAP_INPUT_KEY_R:
      return "KEY_R";
    case TRAP_INPUT_KEY_S:
      return "KEY_S";
    case TRAP_INPUT_KEY_T:
      return "KEY_T";
    case TRAP_INPUT_KEY_U:
      return "KEY_U";
    case TRAP_INPUT_KEY_V:
      return "KEY_V";
    case TRAP_INPUT_KEY_W:
      return "KEY_W";
    case TRAP_INPUT_KEY_X:
      return "KEY_X";
    case TRAP_INPUT_KEY_Y:
      return "KEY_Y";
    case TRAP_INPUT_KEY_Z:
      return "KEY_Z";
    case TRAP_INPUT_KEY_1:
      return "KEY_1";
    case TRAP_INPUT_KEY_2:
      return "KEY_2";
    case TRAP_INPUT_KEY_3:
      return "KEY_3";
    case TRAP_INPUT_KEY_4:
      return "KEY_4";
    case TRAP_INPUT_KEY_5:
      return "KEY_5";
    case TRAP_INPUT_KEY_6:
      return "KEY_6";
    case TRAP_INPUT_KEY_7:
      return "KEY_7";
    case TRAP_INPUT_KEY_8:
      return "KEY_8";
    case TRAP_INPUT_KEY_9:
      return "KEY_9";
    case TRAP_INPUT_KEY_0:
      return "KEY_0";
    case TRAP_INPUT_KEY_RETURN:
      return "KEY_RETURN";
    case TRAP_INPUT_KEY_ESCAPE:
      return "KEY_ESCAPE";
    case TRAP_INPUT_KEY_BACKSPACE:
      return "KEY_BACKSPACE";
    case TRAP_INPUT_KEY_TAB:
      return "KEY_TAB";
    case TRAP_INPUT_KEY_SPACE:
      return "KEY_SPACE";
    case TRAP_INPUT_KEY_MINUS:
      return "KEY_MINUS";
    case TRAP_INPUT_KEY_EQUALS:
      return "KEY_EQUALS";
    case TRAP_INPUT_KEY_LEFTBRACKET:
      return "KEY_LEFTBRACKET";
    case TRAP_INPUT_KEY_RIGHTBRACKET:
      return "KEY_RIGHTBRACKET";
    case TRAP_INPUT_KEY_BACKSLASH:
      return "KEY_BACKSLASH";
    case TRAP_INPUT_KEY_SEMICOLON:
      return "KEY_SEMICOLON";
    case TRAP_INPUT_KEY_APOSTROPHE:
      return "KEY_APOSTROPHE";
    case TRAP_INPUT_KEY_GRAVE:
      return "KEY_GRAVE";
    case TRAP_INPUT_KEY_COMMA:
      return "KEY_COMMA";
    case TRAP_INPUT_KEY_PERIOD:
      return "KEY_PERIOD";
    case TRAP_INPUT_KEY_SLASH:
      return "KEY_SLASH";
    case TRAP_INPUT_KEY_CAPSLOCK:
      return "KEY_CAPSLOCK";
    case TRAP_INPUT_KEY_F1:
      return "KEY_F1";
    case TRAP_INPUT_KEY_F2:
      return "KEY_F2";
    case TRAP_INPUT_KEY_F3:
      return "KEY_F3";
    case TRAP_INPUT_KEY_F4:
      return "KEY_F4";
    case TRAP_INPUT_KEY_F5:
      return "KEY_F5";
    case TRAP_INPUT_KEY_F6:
      return "KEY_F6";
    case TRAP_INPUT_KEY_F7:
      return "KEY_F7";
    case TRAP_INPUT_KEY_F8:
      return "KEY_F8";
    case TRAP_INPUT_KEY_F9:
      return "KEY_F9";
    case TRAP_INPUT_KEY_F10:
      return "KEY_F10";
    case TRAP_INPUT_KEY_F11:
      return "KEY_F11";
    case TRAP_INPUT_KEY_F12:
      return "KEY_F12";
    case TRAP_INPUT_KEY_PRINTSCREEN:
      return "KEY_PRINTSCREEN";
    case TRAP_INPUT_KEY_SCROLLLOCK:
      return "KEY_SCROLLLOCK";
    case TRAP_INPUT_KEY_PAUSE:
      return "KEY_PAUSE";
    case TRAP_INPUT_KEY_INSERT:
      return "KEY_INSERT";
    case TRAP_INPUT_KEY_HOME:
      return "KEY_HOME";
    case TRAP_INPUT_KEY_PAGEUP:
      return "KEY_PAGEUP";
    case TRAP_INPUT_KEY_DELETE:
      return "KEY_DELETE";
    case TRAP_INPUT_KEY_END:
      return "KEY_END";
    case TRAP_INPUT_KEY_PAGEDOWN:
      return "KEY_PAGEDOWN";
    case TRAP_INPUT_KEY_RIGHT:
      return "KEY_RIGHT";
    case TRAP_INPUT_KEY_LEFT:
      return "KEY_LEFT";
    case TRAP_INPUT_KEY_DOWN:
      return "KEY_DOWN";
    case TRAP_INPUT_KEY_UP:
      return "KEY_UP";
    case TRAP_INPUT_KEY_NUMLOCKCLEAR:
      return "KEY_NUMLOCKCLEAR";
    case TRAP_INPUT_KEY_KP_DIVIDE:
      return "KEY_KP_DIVIDE";
    case TRAP_INPUT_KEY_KP_MULTIPLY:
      return "KEY_KP_MULTIPLY";
    case TRAP_INPUT_KEY_KP_MINUS:
      return "KEY_KP_MINUS";
    case TRAP_INPUT_KEY_KP_PLUS:
      return "KEY_KP_PLUS";
    case TRAP_INPUT_KEY_KP_ENTER:
      return "KEY_KP_ENTER";
    case TRAP_INPUT_KEY_KP_1:
      return "KEY_KP_1";
    case TRAP_INPUT_KEY_KP_2:
      return "KEY_KP_2";
    case TRAP_INPUT_KEY_KP_3:
      return "KEY_KP_3";
    case TRAP_INPUT_KEY_KP_4:
      return "KEY_KP_4";
    case TRAP_INPUT_KEY_KP_5:
      return "KEY_KP_5";
    case TRAP_INPUT_KEY_KP_6:
      return "KEY_KP_6";
    case TRAP_INPUT_KEY_KP_7:
      return "KEY_KP_7";
    case TRAP_INPUT_KEY_KP_8:
      return "KEY_KP_8";
    case TRAP_INPUT_KEY_KP_9:
      return "KEY_KP_9";
    case TRAP_INPUT_KEY_KP_0:
      return "KEY_KP_0";
    case TRAP_INPUT_KEY_KP_PERIOD:
      return "KEY_KP_PERIOD";
    case TRAP_INPUT_KEY_NONUSBACKSLASH:
      return "KEY_NONUSBACKSLASH";
    case TRAP_INPUT_KEY_APPLICATION:
      return "KEY_APPLICATION";
    case TRAP_INPUT_KEY_POWER:
      return "KEY_POWER";
    case TRAP_INPUT_KEY_KP_EQUALS:
      return "KEY_KP_EQUALS";
    case TRAP_INPUT_KEY_F13:
      return "KEY_F13";
    case TRAP_INPUT_KEY_F14:
      return "KEY_F14";
    case TRAP_INPUT_KEY_F15:
      return "KEY_F15";
    case TRAP_INPUT_KEY_F16:
      return "KEY_F16";
    case TRAP_INPUT_KEY_F17:
      return "KEY_F17";
    case TRAP_INPUT_KEY_F18:
      return "KEY_F18";
    case TRAP_INPUT_KEY_F19:
      return "KEY_F19";
    case TRAP_INPUT_KEY_F20:
      return "KEY_F20";
    case TRAP_INPUT_KEY_F21:
      return "KEY_F21";
    case TRAP_INPUT_KEY_F22:
      return "KEY_F22";
    case TRAP_INPUT_KEY_F23:
      return "KEY_F23";
    case TRAP_INPUT_KEY_F24:
      return "KEY_F24";
    case TRAP_INPUT_KEY_EXECUTE:
      return "KEY_EXECUTE";
    case TRAP_INPUT_KEY_HELP:
      return "KEY_HELP";
    case TRAP_INPUT_KEY_MENU:
      return "KEY_MENU";
    case TRAP_INPUT_KEY_SELECT:
      return "KEY_SELECT";
    case TRAP_INPUT_KEY_STOP:
      return "KEY_STOP";
    case TRAP_INPUT_KEY_AGAIN:
      return "KEY_AGAIN";
    case TRAP_INPUT_KEY_UNDO:
      return "KEY_UNDO";
    case TRAP_INPUT_KEY_CUT:
      return "KEY_CUT";
    case TRAP_INPUT_KEY_COPY:
      return "KEY_COPY";
    case TRAP_INPUT_KEY_PASTE:
      return "KEY_PASTE";
    case TRAP_INPUT_KEY_FIND:
      return "KEY_FIND";
    case TRAP_INPUT_KEY_MUTE:
      return "KEY_MUTE";
    case TRAP_INPUT_KEY_VOLUMEUP:
      return "KEY_VOLUMEUP";
    case TRAP_INPUT_KEY_VOLUMEDOWN:
      return "KEY_VOLUMEDOWN";
    case TRAP_INPUT_KEY_KP_COMMA:
      return "KEY_KP_COMMA";
    case TRAP_INPUT_KEY_KP_EQUALSAS400:
      return "KEY_KP_EQUALSAS400";
    case TRAP_INPUT_KEY_INTERNATIONAL1:
      return "KEY_INTERNATIONAL1";
    case TRAP_INPUT_KEY_INTERNATIONAL2:
      return "KEY_INTERNATIONAL2";
    case TRAP_INPUT_KEY_INTERNATIONAL3:
      return "KEY_INTERNATIONAL3";
    case TRAP_INPUT_KEY_INTERNATIONAL4:
      return "KEY_INTERNATIONAL4";
    case TRAP_INPUT_KEY_INTERNATIONAL5:
      return "KEY_INTERNATIONAL5";
    case TRAP_INPUT_KEY_INTERNATIONAL6:
      return "KEY_INTERNATIONAL6";
    case TRAP_INPUT_KEY_INTERNATIONAL7:
      return "KEY_INTERNATIONAL7";
    case TRAP_INPUT_KEY_INTERNATIONAL8:
      return "KEY_INTERNATIONAL8";
    case TRAP_INPUT_KEY_INTERNATIONAL9:
      return "KEY_INTERNATIONAL9";
    case TRAP_INPUT_KEY_LANG1:
      return "KEY_LANG1";
    case TRAP_INPUT_KEY_LANG2:
      return "KEY_LANG2";
    case TRAP_INPUT_KEY_LANG3:
      return "KEY_LANG3";
    case TRAP_INPUT_KEY_LANG4:
      return "KEY_LANG4";
    case TRAP_INPUT_KEY_LANG5:
      return "KEY_LANG5";
    case TRAP_INPUT_KEY_LANG6:
      return "KEY_LANG6";
    case TRAP_INPUT_KEY_LANG7:
      return "KEY_LANG7";
    case TRAP_INPUT_KEY_LANG8:
      return "KEY_LANG8";
    case TRAP_INPUT_KEY_LANG9:
      return "KEY_LANG9";
    case TRAP_INPUT_KEY_ALTERASE:
      return "KEY_ALTERASE";
    case TRAP_INPUT_KEY_SYSREQ:
      return "KEY_SYSREQ";
    case TRAP_INPUT_KEY_CANCEL:
      return "KEY_CANCEL";
    case TRAP_INPUT_KEY_CLEAR:
      return "KEY_CLEAR";
    case TRAP_INPUT_KEY_PRIOR:
      return "KEY_PRIOR";
    case TRAP_INPUT_KEY_RETURN2:
      return "KEY_RETURN2";
    case TRAP_INPUT_KEY_SEPARATOR:
      return "KEY_SEPARATOR";
    case TRAP_INPUT_KEY_OUT:
      return "KEY_OUT";
    case TRAP_INPUT_KEY_OPER:
      return "KEY_OPER";
    case TRAP_INPUT_KEY_CLEARAGAIN:
      return "KEY_CLEARAGAIN";
    case TRAP_INPUT_KEY_CRSEL:
      return "KEY_CRSEL";
    case TRAP_INPUT_KEY_EXSEL:
      return "KEY_EXSEL";
    case TRAP_INPUT_KEY_KP_00:
      return "KEY_KP_00";
    case TRAP_INPUT_KEY_KP_000:
      return "KEY_KP_000";
    case TRAP_INPUT_KEY_THOUSANDSSEPARATOR:
      return "KEY_THOUSANDSSEPARATOR";
    case TRAP_INPUT_KEY_DECIMALSEPARATOR:
      return "KEY_DECIMALSEPARATOR";
    case TRAP_INPUT_KEY_CURRENCYUNIT:
      return "KEY_CURRENCYUNIT";
    case TRAP_INPUT_KEY_CURRENCYSUBUNIT:
      return "KEY_CURRENCYSUBUNIT";
    case TRAP_INPUT_KEY_KP_LEFTPAREN:
      return "KEY_KP_LEFTPAREN";
    case TRAP_INPUT_KEY_KP_RIGHTPAREN:
      return "KEY_KP_RIGHTPAREN";
    case TRAP_INPUT_KEY_KP_LEFTBRACE:
      return "KEY_KP_LEFTBRACE";
    case TRAP_INPUT_KEY_KP_RIGHTBRACE:
      return "KEY_KP_RIGHTBRACE";
    case TRAP_INPUT_KEY_KP_TAB:
      return "KEY_KP_TAB";
    case TRAP_INPUT_KEY_KP_BACKSPACE:
      return "KEY_KP_BACKSPACE";
    case TRAP_INPUT_KEY_KP_A:
      return "KEY_KP_A";
    case TRAP_INPUT_KEY_KP_B:
      return "KEY_KP_B";
    case TRAP_INPUT_KEY_KP_C:
      return "KEY_KP_C";
    case TRAP_INPUT_KEY_KP_D:
      return "KEY_KP_D";
    case TRAP_INPUT_KEY_KP_E:
      return "KEY_KP_E";
    case TRAP_INPUT_KEY_KP_F:
      return "KEY_KP_F";
    case TRAP_INPUT_KEY_KP_XOR:
      return "KEY_KP_XOR";
    case TRAP_INPUT_KEY_KP_POWER:
      return "KEY_KP_POWER";
    case TRAP_INPUT_KEY_KP_PERCENT:
      return "KEY_KP_PERCENT";
    case TRAP_INPUT_KEY_KP_LESS:
      return "KEY_KP_LESS";
    case TRAP_INPUT_KEY_KP_GREATER:
      return "KEY_KP_GREATER";
    case TRAP_INPUT_KEY_KP_AMPERSAND:
      return "KEY_KP_AMPERSAND";
    case TRAP_INPUT_KEY_KP_DBLAMPERSAND:
      return "KEY_KP_DBLAMPERSAND";
    case TRAP_INPUT_KEY_KP_VERTICALBAR:
      return "KEY_KP_VERTICALBAR";
    case TRAP_INPUT_KEY_KP_DBLVERTICALBAR:
      return "KEY_KP_DBLVERTICALBAR";
    case TRAP_INPUT_KEY_KP_COLON:
      return "KEY_KP_COLON";
    case TRAP_INPUT_KEY_KP_HASH:
      return "KEY_KP_HASH";
    case TRAP_INPUT_KEY_KP_SPACE:
      return "KEY_KP_SPACE";
    case TRAP_INPUT_KEY_KP_AT:
      return "KEY_KP_AT";
    case TRAP_INPUT_KEY_KP_EXCLAM:
      return "KEY_KP_EXCLAM";
    case TRAP_INPUT_KEY_KP_MEMSTORE:
      return "KEY_KP_MEMSTORE";
    case TRAP_INPUT_KEY_KP_MEMRECALL:
      return "KEY_KP_MEMRECALL";
    case TRAP_INPUT_KEY_KP_MEMCLEAR:
      return "KEY_KP_MEMCLEAR";
    case TRAP_INPUT_KEY_KP_MEMADD:
      return "KEY_KP_MEMADD";
    case TRAP_INPUT_KEY_KP_MEMSUBTRACT:
      return "KEY_KP_MEMSUBTRACT";
    case TRAP_INPUT_KEY_KP_MEMMULTIPLY:
      return "KEY_KP_MEMMULTIPLY";
    case TRAP_INPUT_KEY_KP_MEMDIVIDE:
      return "KEY_KP_MEMDIVIDE";
    case TRAP_INPUT_KEY_KP_PLUSMINUS:
      return "KEY_KP_PLUSMINUS";
    case TRAP_INPUT_KEY_KP_CLEAR:
      return "KEY_KP_CLEAR";
    case TRAP_INPUT_KEY_KP_CLEARENTRY:
      return "KEY_KP_CLEARENTRY";
    case TRAP_INPUT_KEY_KP_BINARY:
      return "KEY_KP_BINARY";
    case TRAP_INPUT_KEY_KP_OCTAL:
      return "KEY_KP_OCTAL";
    case TRAP_INPUT_KEY_KP_DECIMAL:
      return "KEY_KP_DECIMAL";
    case TRAP_INPUT_KEY_KP_HEXADECIMAL:
      return "KEY_KP_HEXADECIMAL";
    case TRAP_INPUT_KEY_LCTRL:
      return "KEY_LCTRL";
    case TRAP_INPUT_KEY_LSHIFT:
      return "KEY_LSHIFT";
    case TRAP_INPUT_KEY_LALT:
      return "KEY_LALT";
    case TRAP_INPUT_KEY_LGUI:
      return "KEY_LGUI";
    case TRAP_INPUT_KEY_RCTRL:
      return "KEY_RCTRL";
    case TRAP_INPUT_KEY_RSHIFT:
      return "KEY_RSHIFT";
    case TRAP_INPUT_KEY_RALT:
      return "KEY_RALT";
    case TRAP_INPUT_KEY_RGUI:
      return "KEY_RGUI";
    case TRAP_INPUT_KEY_MODE:
      return "KEY_MODE";
    case TRAP_INPUT_KEY_AUDIONEXT:
      return "KEY_AUDIONEXT";
    case TRAP_INPUT_KEY_AUDIOPREV:
      return "KEY_AUDIOPREV";
    case TRAP_INPUT_KEY_AUDIOSTOP:
      return "KEY_AUDIOSTOP";
    case TRAP_INPUT_KEY_AUDIOPLAY:
      return "KEY_AUDIOPLAY";
    case TRAP_INPUT_KEY_AUDIOMUTE:
      return "KEY_AUDIOMUTE";
    case TRAP_INPUT_KEY_MEDIASELECT:
      return "KEY_MEDIASELECT";
    case TRAP_INPUT_KEY_WWW:
      return "KEY_WWW";
    case TRAP_INPUT_KEY_MAIL:
      return "KEY_MAIL";
    case TRAP_INPUT_KEY_CALCULATOR:
      return "KEY_CALCULATOR";
    case TRAP_INPUT_KEY_COMPUTER:
      return "KEY_COMPUTER";
    case TRAP_INPUT_KEY_AC_SEARCH:
      return "KEY_AC_SEARCH";
    case TRAP_INPUT_KEY_AC_HOME:
      return "KEY_AC_HOME";
    case TRAP_INPUT_KEY_AC_BACK:
      return "KEY_AC_BACK";
    case TRAP_INPUT_KEY_AC_FORWARD:
      return "KEY_AC_FORWARD";
    case TRAP_INPUT_KEY_AC_STOP:
      return "KEY_AC_STOP";
    case TRAP_INPUT_KEY_AC_REFRESH:
      return "KEY_AC_REFRESH";
    case TRAP_INPUT_KEY_AC_BOOKMARKS:
      return "KEY_AC_BOOKMARKS";
    case TRAP_INPUT_KEY_BRIGHTNESSDOWN:
      return "KEY_BRIGHTNESSDOWN";
    case TRAP_INPUT_KEY_BRIGHTNESSUP:
      return "KEY_BRIGHTNESSUP";
    case TRAP_INPUT_KEY_DISPLAYSWITCH:
      return "KEY_DISPLAYSWITCH";
    case TRAP_INPUT_KEY_KBDILLUMTOGGLE:
      return "KEY_KBDILLUMTOGGLE";
    case TRAP_INPUT_KEY_KBDILLUMDOWN:
      return "KEY_KBDILLUMDOWN";
    case TRAP_INPUT_KEY_KBDILLUMUP:
      return "KEY_KBDILLUMUP";
    case TRAP_INPUT_KEY_EJECT:
      return "KEY_EJECT";
    case TRAP_INPUT_KEY_SLEEP:
      return "KEY_SLEEP";
    case TRAP_INPUT_KEY_APP1:
      return "KEY_APP1";
    case TRAP_INPUT_KEY_APP2:
      return "KEY_APP2";
    case TRAP_INPUT_KEY_AUDIOREWIND:
      return "KEY_AUDIOREWIND";
    case TRAP_INPUT_KEY_AUDIOFASTFORWARD:
      return "KEY_AUDIOFASTFORWARD";
    case TRAP_INPUT_KEY_SOFTLEFT:
      return "KEY_SOFTLEFT";
    case TRAP_INPUT_KEY_SOFTRIGHT:
      return "KEY_SOFTRIGHT";
    case TRAP_INPUT_KEY_CALL:
      return "KEY_CALL";
    case TRAP_INPUT_KEY_ENDCALL:
      return "KEY_ENDCALL";
      default:
        return NULL;
    }
}
