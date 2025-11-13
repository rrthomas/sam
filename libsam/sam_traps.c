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
#include <math.h>

#include <SDL.h>
#include <SDL2_gfxPrimitives.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"

// Division macros
#define DIV_CATCH_ZERO(a, b) ((b) == 0 ? 0 : (a) / (b))
#define MOD_CATCH_ZERO(a, b) ((b) == 0 ? (a) : (a) % (b))

// Adapted from https://stackoverflow.com/questions/101439/the-most-efficient-way-to-implement-an-integer-based-power-function-powint-int
static sam_uword_t powi(sam_uword_t base, sam_uword_t exp)
{
    sam_uword_t result = 1;
    for (;;) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        if (exp == 0)
            break;
        base *= base;
    }

    return result;
}

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
#ifdef SAM_DEBUG
    debug("%s\n", trap_name(function));
#endif
    switch (function) {
    case TRAP_NOP:
        break;
    case TRAP_I2F:
        {
            sam_word_t i;
            POP_INT(i);
            PUSH_FLOAT((sam_float_t)i);
        }
        break;
    case TRAP_F2I:
        {
            sam_float_t f;
            POP_FLOAT(f);
            PUSH_INT((sam_word_t)rint(f));
        }
        break;
    case TRAP_POP:
        if (sam_sp == 0)
            HALT(SAM_ERROR_STACK_UNDERFLOW);
        sam_uword_t size;
        HALT_IF_ERROR(sam_stack_item(-1, &sam_sp, &size));
        break;
    case TRAP_GET:
        {
            sam_word_t pos;
            POP_INT(pos);
            sam_uword_t addr, size;
            HALT_IF_ERROR(sam_stack_item(pos, &addr, &size));
            sam_uword_t opcode;
            HALT_IF_ERROR(sam_stack_peek(addr, &opcode));
            opcode &= SAM_OP_MASK;
            if (opcode == SAM_INSN_STACK)
                PUSH_LINK(addr);
            else {
                sam_uword_t i;
                for (i = 0; i < size; i++) {
                    sam_uword_t temp;
                    HALT_IF_ERROR(sam_stack_peek(addr + i, &temp));
                    PUSH_WORD(temp);
                }
            }
        }
        break;
    case TRAP_SET:
        {
            sam_word_t pos;
            POP_INT(pos);
            sam_uword_t addr1, size1, addr2, size2;
            HALT_IF_ERROR(sam_stack_item(-1, &addr1, &size1));
            HALT_IF_ERROR(sam_stack_item(pos, &addr2, &size2));
            sam_stack_set(addr1, size1, addr2, size2);
            sam_sp -= size2;
        }
        break;
    case TRAP_IGET:
        // TODO: GET an inner item (takes stack index and inner index)
        break;
    case TRAP_ISET:
        // TODO: SET an inner item (takes stack index and inner index)
        break;
    case TRAP_DO:
        {
            sam_uword_t code;
            POP_WORD((sam_word_t *)&code);
            HALT_IF_ERROR(sam_find_code(code, &code));
            DO(code);
        }
        break;
    case TRAP_IF:
        {
            sam_uword_t then, else_;
            POP_WORD((sam_word_t *)&else_);
            POP_WORD((sam_word_t *)&then);
            HALT_IF_ERROR(sam_find_code(then, &then));
            HALT_IF_ERROR(sam_find_code(else_, &else_));
            sam_word_t flag;
            POP_INT(flag);
            DO(flag ? then : else_);
        }
        break;
    case TRAP_WHILE:
        {
            sam_word_t flag;
            POP_INT(flag);
            if (!flag)
                RET;
        }
        break;
    case TRAP_LOOP:
        sam_pc = sam_pc0;
        break;
    case TRAP_NOT:
        {
            sam_word_t a;
            POP_INT(a);
            PUSH_INT(~a);
        }
        break;
    case TRAP_AND:
        {
            sam_word_t a, b;
            POP_INT(a);
            POP_INT(b);
            PUSH_INT(a & b);
        }
        break;
    case TRAP_OR:
        {
            sam_word_t a, b;
            POP_INT(a);
            POP_INT(b);
            PUSH_INT(a | b);
        }
        break;
    case TRAP_XOR:
        {
            sam_word_t a, b;
            POP_INT(a);
            POP_INT(b);
            PUSH_INT(a ^ b);
        }
        break;
    case TRAP_LSH:
        {
            sam_word_t shift, value;
            POP_INT(shift);
            POP_INT(value);
            PUSH_INT(shift < (sam_word_t)SAM_WORD_BIT ? LSHIFT(value, shift) : 0);
        }
        break;
    case TRAP_RSH:
        {
            sam_word_t shift, value;
            POP_INT(shift);
            POP_INT(value);
            PUSH_INT(shift < (sam_word_t)SAM_WORD_BIT ? (sam_word_t)((sam_uword_t)value >> shift) : 0);
        }
        break;
    case TRAP_ARSH:
        {
            sam_word_t shift, value;
            POP_INT(shift);
            POP_INT(value);
            PUSH_INT(ARSHIFT(value, shift));
        }
        break;
    case TRAP_EQ:
        {
            sam_word_t x;
            POP_WORD(&x);
            switch (x & SAM_OP_MASK) {
            case SAM_INSN_INT:
            case SAM_INSN_LINK:
                {
                    sam_word_t y;
                    POP_WORD(&y);
                    PUSH_INT(x == y);
                }
                break;
            case SAM_INSN__PUSH:
            case SAM_INSN__FLOAT:
                {
                    sam_word_t x2, y, y2;
                    POP_WORD(&x2);
                    if ((x & SAM_OP_MASK) == SAM_INSN__PUSH) {
                        if ((x2 & SAM_OP_MASK) != SAM_INSN_PUSH)
                            HALT(SAM_ERROR_UNPAIRED_PUSH);
                    } else if ((x & SAM_OP_MASK) == SAM_INSN__FLOAT) {
                        if ((x2 & SAM_OP_MASK) != SAM_INSN_FLOAT)
                            HALT(SAM_ERROR_UNPAIRED_FLOAT);
                    }
                    POP_WORD(&y);
                    POP_WORD(&y2);
                    PUSH_INT(x == y && x2 == y2);
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_NUMBER);
                break;
            }
        }
        break;
    case TRAP_LT:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
            switch (operand & SAM_OP_MASK) {
            case SAM_INSN_INT:
                {
                    sam_word_t a, b;
                    POP_INT(a);
                    POP_INT(b);
                    PUSH_INT(a < b);
                }
                break;
            case SAM_INSN__FLOAT:
                {
                    sam_float_t a, b;
                    POP_FLOAT(a);
                    POP_FLOAT(b);
                    PUSH_INT(a < b);
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_NUMBER);
                break;
            }
        }
        break;
    case TRAP_NEG:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
            switch (operand & SAM_OP_MASK) {
            case SAM_INSN_INT:
                {
                    sam_uword_t a;
                    POP_UINT(a);
                    PUSH_INT(-a);
                }
                break;
            case SAM_INSN__FLOAT:
                {
                    sam_float_t a;
                    POP_FLOAT(a);
                    PUSH_FLOAT(-a);
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_NUMBER);
                break;
            }
        }
        break;
    case TRAP_ADD:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
            switch (operand & SAM_OP_MASK) {
            case SAM_INSN_INT:
                {
                    sam_uword_t a, b;
                    POP_UINT(a);
                    POP_UINT(b);
                    PUSH_INT((sam_word_t)(b + a));
                }
                break;
            case SAM_INSN__FLOAT:
                {
                    sam_float_t a, b;
                    POP_FLOAT(a);
                    POP_FLOAT(b);
                    PUSH_FLOAT(a + b);
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_NUMBER);
                break;
            }
        }
        break;
    case TRAP_MUL:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
            switch (operand & SAM_OP_MASK) {
            case SAM_INSN_INT:
                {
                    sam_uword_t a, b;
                    POP_UINT(a);
                    POP_UINT(b);
                    PUSH_INT((sam_word_t)(a * b));
                }
                break;
            case SAM_INSN__FLOAT:
                {
                    sam_float_t a, b;
                    POP_FLOAT(a);
                    POP_FLOAT(b);
                    PUSH_FLOAT(a * b);
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_NUMBER);
                break;
            }
        }
        break;
    case TRAP_DIV:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
            switch (operand & SAM_OP_MASK) {
            case SAM_INSN_INT:
                {
                    sam_word_t divisor, dividend;
                    POP_INT(divisor);
                    POP_INT(dividend);
                    if (dividend == SAM_WORD_MIN && divisor == -1) {
                        PUSH_INT(SAM_INT_MIN);
                    } else {
                        PUSH_INT(DIV_CATCH_ZERO(dividend, divisor));
                    }
                }
                break;
            case SAM_INSN__FLOAT:
                {
                    sam_float_t divisor, dividend;
                    POP_FLOAT(divisor);
                    POP_FLOAT(dividend);
                    PUSH_FLOAT(DIV_CATCH_ZERO(dividend, divisor));
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_NUMBER);
                break;
            }
        }
        break;
    case TRAP_REM:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
            switch (operand & SAM_OP_MASK) {
            case SAM_INSN_INT:
                {
                    sam_uword_t divisor, dividend;
                    POP_UINT(divisor);
                    POP_UINT(dividend);
                    PUSH_INT(MOD_CATCH_ZERO(dividend, divisor));
                }
                break;
            case SAM_INSN__FLOAT:
                {
                    sam_float_t divisor, dividend;
                    POP_FLOAT(divisor);
                    POP_FLOAT(dividend);
                    PUSH_FLOAT(divisor == 0 ? dividend : fmodf(divisor, dividend));
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_NUMBER);
                break;
            }
        }
        break;
    case TRAP_POW:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
            switch (operand & SAM_OP_MASK) {
            case SAM_INSN_INT:
                {
                    sam_uword_t a, b;
                    POP_UINT(a);
                    POP_UINT(b);
                    PUSH_INT(powi(a, b));
                }
                break;
            case SAM_INSN__FLOAT:
                {
                    sam_float_t a, b;
                    POP_FLOAT(a);
                    POP_FLOAT(b);
                    PUSH_FLOAT(powf(a, b));
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_NUMBER);
                break;
            }
        }
        break;
    case TRAP_SIN:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
            switch (operand & SAM_OP_MASK) {
            case SAM_INSN__FLOAT:
                {
                    sam_float_t a;
                    POP_FLOAT(a);
                    PUSH_FLOAT(sinf(a));
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_FLOAT);
                break;
            }
        }
        break;
    case TRAP_COS:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
            switch (operand & SAM_OP_MASK) {
            case SAM_INSN__FLOAT:
                {
                    sam_float_t a;
                    POP_FLOAT(a);
                    PUSH_FLOAT(cosf(a));
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_FLOAT);
                break;
            }
        }
        break;
    case TRAP_DEG:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
            switch (operand & SAM_OP_MASK) {
            case SAM_INSN__FLOAT:
                {
                    sam_float_t a;
                    POP_FLOAT(a);
                    PUSH_FLOAT(a * (M_1_PI * 180.0));
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_FLOAT);
                break;
            }
        }
        break;
    case TRAP_RAD:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
            switch (operand & SAM_OP_MASK) {
            case SAM_INSN__FLOAT:
                {
                    sam_float_t a;
                    POP_FLOAT(a);
                    PUSH_FLOAT(a * (M_PI / 180.0));
                }
                break;
            default:
                HALT(SAM_ERROR_NOT_FLOAT);
                break;
            }
        }
        break;
    case TRAP_HALT:
        if (sam_sp < 1)
            HALT(SAM_ERROR_STACK_UNDERFLOW);
        else {
            sam_uword_t ret;
            POP_INT(ret);
            HALT(SAM_ERROR_HALT | ret << SAM_OP_SHIFT);
        }
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
            pixelRGBA(ren, x, y, color, color, color, SDL_ALPHA_OPAQUE);
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
            lineRGBA(ren, x1, y1, x2, y2, color, color, color, SDL_ALPHA_OPAQUE);
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
            rectangleRGBA(ren, x, y, x + width - 1, y + height - 1, color, color, color, SDL_ALPHA_OPAQUE);
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
            boxRGBA(ren, x, y, x + width - 1, y + height - 1, color, color, color, SDL_ALPHA_OPAQUE);
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
            circleRGBA(ren, xCenter, yCenter, radius, color, color, color, SDL_ALPHA_OPAQUE);
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
            filledCircleRGBA(ren, xCenter, yCenter, radius, color, color, color, SDL_ALPHA_OPAQUE);
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

void sam_traps_wait(void) {
    while (sam_traps_process_events() == 0) {
        SDL_Delay(sam_update_interval);
    }
}
