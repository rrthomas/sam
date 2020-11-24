// The traps.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include "glcd.h"

extern "C" {
#include "minmax.h"

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"
#include "sam_traps.h"
}

sam_word_t sam_trap(sam_uword_t function)
{
    int error = SAM_ERROR_OK;
    switch (function) {
    case TRAP_BLACK:
        PUSH_INT(BLACK);
        break;
    case TRAP_WHITE:
        PUSH_INT(WHITE);
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
            GLCD.ClearScreen(color);
        }
        break;
    case TRAP_SETDOT:
        {
            sam_word_t x, y, color;
            POP_UINT(color);
            POP_UINT(y);
            POP_UINT(x);
            GLCD.SetDot(x, y, color);
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
            if (x1 == x2)
                GLCD.DrawVLine(x1, MIN(y1, y2), MAX(y1, y2) - MIN (y1, y2), color);
            else if (y1 == y2)
                GLCD.DrawHLine(MIN(x1, x2), y1, MAX(x1, x2) - MIN (x1, x2), color);
            else
                GLCD.DrawLine(x1, y1, x2, y2, color);
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
            if (height > 0 & width > 0)
                GLCD.DrawRect(x, y, width - 1, height - 1, color);
        }
        break;
    case TRAP_DRAWROUNDRECT:
        {
            sam_word_t x, y, width, height, radius, color;
            POP_UINT(color);
            POP_UINT(radius);
            if (radius > 0)
                radius--;
            POP_UINT(height);
            POP_UINT(width);
            POP_UINT(y);
            POP_UINT(x);
            if (height > 0 && width > 0 && radius >= 0)
                GLCD.DrawRoundRect(x, y, width - 1, height - 1, radius, color);
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
            GLCD.FillRect(x, y, width, height, color);
        }
        break;
    case TRAP_INVERTRECT:
        {
            sam_word_t x, y, width, height;
            POP_UINT(height);
            POP_UINT(width);
            POP_UINT(y);
            POP_UINT(x);
            // N.B. EMFCamp version of GLCD appears to mis-implement the spec.
            if (height > 0 & width > 0)
                GLCD.InvertRect(x, y, width - 1, height - 1);
        }
        break;
    case TRAP_DRAWCIRCLE:
        {
            sam_word_t xCenter, yCenter, radius, color;
            POP_UINT(color);
            POP_UINT(radius);
            POP_UINT(yCenter);
            POP_UINT(xCenter);
            GLCD.DrawCircle(xCenter, yCenter, radius, color);
        }
        break;
    case TRAP_FILLCIRCLE:
        {
            sam_word_t xCenter, yCenter, radius, color;
            POP_UINT(color);
            POP_UINT(radius);
            POP_UINT(yCenter);
            POP_UINT(xCenter);
            GLCD.FillCircle(xCenter, yCenter, radius, color);
        }
        break;
    case TRAP_DRAWBITMAP:
        {
            sam_word_t bitmap, x, y, color;
            POP_UINT(color);
            POP_UINT(y);
            POP_UINT(x);
            POP_UINT(bitmap);
            GLCD.DrawBitmap((Image_t)bitmap, x, y, color);
        }
        break;
    default:
        error = SAM_ERROR_INVALID_FUNCTION;
        break;
    }

 error:
    return error;
}
