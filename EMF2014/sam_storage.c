// Allocate storage for the registers and memory.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sam.h"
#include "sam_opcodes.h"

#include "sam_private.h"


// VM registers

#define R(reg, type) \
    type sam_##reg;
#include "sam_registers.h"
#undef R


// Stack

// Return the offset of stack item n from the top.
sam_uword_t sam_stack_item(sam_word_t *m0, sam_uword_t msize, sam_uword_t s0, sam_uword_t sp, sam_uword_t n)
{
    if (sp <= s0)
        return SAM_ERROR_STACK_UNDERFLOW;
    else if (sp > msize)
        return SAM_ERROR_STACK_OVERFLOW;

    sam_uword_t i;
    for (i = 0; i <= n; i++) {
        sam_uword_t inst = m0[--sp];
        // If instruction is a KET, skip to matching BRA.
        if ((inst & SAM_OP_MASK) == SAM_INSN_KET) {
            sp -= inst >> SAM_OP_SHIFT + 1;
            assert((m0[sp] & SAM_OP_MASK) == SAM_INSN_BRA); // FIXME: THROW
        }
    }
    return sp;
}

// Given a LINK instruction, find the corresponding BRA instruction, and
// return the stack address of the first code word.
sam_uword_t sam_find_code(sam_uword_t code)
{
    sam_uword_t inst = code & SAM_OP_MASK;
    sam_uword_t n;
    assert(inst == SAM_INSN_LINK); // FIXME: THROW(SAM_ERROR_NOT_CODE)
    do {
        n = code >> SAM_OP_SHIFT;
        code = sam_m0[n];
        inst = code & SAM_OP_MASK;
    } while (inst == SAM_INSN_LINK);
    assert(inst == SAM_INSN_BRA); // FIXME: THROW(SAM_ERROR_NOT_CODE)
    return n + 1;
}

int sam_pop_stack(sam_word_t *m0, sam_uword_t msize, sam_uword_t s0, sam_uword_t *sp, sam_word_t *val_ptr)
{
    *sp = sam_stack_item(m0, msize, s0, *sp, 0);
    *val_ptr = m0[*sp];
    return SAM_ERROR_OK;
}

int sam_push_stack(sam_word_t *m0, sam_uword_t msize, sam_uword_t s0, sam_uword_t *sp, sam_word_t val)
{
    if (*sp >= msize)
        return SAM_ERROR_STACK_OVERFLOW;
    m0[*sp] = val;
    (*sp)++;
    return SAM_ERROR_OK;
}


// Initialise VM state.
int sam_init(sam_word_t *m0, sam_uword_t msize, sam_uword_t sp)
{
    if ((sam_m0 = m0) == NULL)
        return -1;
    sam_msize = msize;
    sam_sp = sp;
    sam_s0 = sam_handler_sp = 0;

    return 0;
}
