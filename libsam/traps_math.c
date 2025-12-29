// SAM's math traps.
//
// (c) Reuben Thomas 2020-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <math.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "private.h"
#include "run.h"
#include "traps_math.h"

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

sam_word_t sam_math_trap(sam_uword_t function)
{
    int error = SAM_ERROR_OK;
    switch (function) {
    case TRAP_MATH_POW:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_stack->sp - 1, &operand));
            if ((operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                sam_uword_t a, b;
                POP_UINT(b);
                POP_UINT(a);
                PUSH_INT(powi(a, b));
            } else if ((operand & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
                sam_float_t a, b;
                POP_FLOAT(b);
                POP_FLOAT(a);
                PUSH_FLOAT(powf(a, b));
            } else
                HALT(SAM_ERROR_WRONG_TYPE);
        }
        break;
    case TRAP_MATH_SIN:
        {
            sam_float_t a;
            POP_FLOAT(a);
            PUSH_FLOAT(sinf(a));
        }
        break;
    case TRAP_MATH_COS:
        {
            sam_float_t a;
            POP_FLOAT(a);
            PUSH_FLOAT(cosf(a));
        }
        break;
    case TRAP_MATH_DEG:
        {
            sam_float_t a;
            POP_FLOAT(a);
            PUSH_FLOAT(a * (M_1_PI * 180.0));
        }
        break;
    case TRAP_MATH_RAD:
        {
            sam_float_t a;
            POP_FLOAT(a);
            PUSH_FLOAT(a * (M_PI / 180.0));
        }
        break;
    }
 error:
    return error;
}

char *sam_math_trap_name(sam_word_t function) {
    switch (function) {
    case TRAP_MATH_POW:
        return "POW";
    case TRAP_MATH_SIN:
        return "SIN";
    case TRAP_MATH_COS:
        return "COS";
    case TRAP_MATH_DEG:
        return "DEG";
    case TRAP_MATH_RAD:
        return "RAD";
    default:
        return NULL;
    }
}
