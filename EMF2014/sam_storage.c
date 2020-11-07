// Allocate storage for the registers and memory.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <stdlib.h>
#include <string.h>

#include "sam.h"

#include "sam_private.h"


// VM registers

#define R(reg, type) \
    type sam_##reg;
#include "sam_registers.h"
#undef R


// Stacks

int sam_pop_stack(sam_word_t *s0, sam_uword_t ssize, sam_uword_t *sp, sam_word_t *val_ptr)
{
    if (unlikely(*sp == 0))
        return SAM_ERROR_STACK_UNDERFLOW;
    else if (unlikely(*sp > ssize))
        return SAM_ERROR_STACK_OVERFLOW;
    (*sp)--;
    *val_ptr = s0[*sp];
    return SAM_ERROR_OK;
}

int sam_push_stack(sam_word_t *s0, sam_uword_t ssize, sam_uword_t *sp, sam_word_t val)
{
    if (unlikely(*sp >= ssize))
        return SAM_ERROR_STACK_OVERFLOW;
    s0[*sp] = val;
    (*sp)++;
    return SAM_ERROR_OK;
}


// Initialise VM state.
int sam_init(sam_word_t *buf, sam_uword_t stack_size)
{
    if (buf == NULL)
        return -1;
    sam_pc = sam_s0 = buf;
    sam_ssize = stack_size;
    sam_handler_sp = sam_sp = 0;

    return 0;
}
