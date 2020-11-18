// Allocate storage for the registers and memory.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

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
int sam_stack_item(sam_uword_t n, sam_uword_t *addr, sam_uword_t *size)
{
    sam_uword_t sp = sam_sp, last_opcode = 0;
    sam_uword_t i, last_sp;
    for (i = 0; i <= n; i++) {
        last_sp = sp;
        if (sp == 0)
            return SAM_ERROR_STACK_UNDERFLOW;
        sam_uword_t inst = sam_s0[--sp];
        sam_uword_t opcode = inst & SAM_OP_MASK;
        // If instruction is a KET, skip to matching BRA.
        if (opcode == SAM_INSN_KET) {
            sp -= inst >> SAM_OP_SHIFT + 1;
            if ((sam_s0[sp] & SAM_OP_MASK) != SAM_INSN_BRA)
                return SAM_ERROR_BAD_BRACKET;
        } else if (opcode == SAM_INSN__PUSH) {
            if (sam_s0[--sp] & SAM_OP_MASK != SAM_INSN_PUSH)
                return SAM_ERROR_UNPAIRED_PUSH;
        } else if (opcode == SAM_INSN_PUSH && last_opcode != SAM_INSN__PUSH)
            return SAM_ERROR_UNPAIRED_PUSH;
        else if (opcode == SAM_INSN__FLOAT) {
            if (sam_s0[--sp] & SAM_OP_MASK != SAM_INSN_FLOAT)
                return SAM_ERROR_UNPAIRED_FLOAT;
        } else if (opcode == SAM_INSN_FLOAT && last_opcode != SAM_INSN__FLOAT)
            return SAM_ERROR_UNPAIRED_FLOAT;
        last_opcode = opcode;
    }
    *addr = sp;
    *size = last_sp - sp;
    return SAM_ERROR_OK;
}

// Given a LINK instruction, find the corresponding BRA instruction, and
// return the stack address of the first code word.
int sam_find_code(sam_uword_t code, sam_uword_t *addr)
{
    sam_uword_t inst = code & SAM_OP_MASK;
    if (inst != SAM_INSN_LINK)
        return SAM_ERROR_NOT_CODE;
    do {
        *addr = code >> SAM_OP_SHIFT;
        code = sam_s0[*addr];
        inst = code & SAM_OP_MASK;
    } while (inst == SAM_INSN_LINK);
    if (inst != SAM_INSN_BRA)
        return SAM_ERROR_NOT_CODE;
    (*addr)++;
    return SAM_ERROR_OK;
}

int sam_pop_stack(sam_word_t *val_ptr)
{
    if (sam_sp == 0)
        return SAM_ERROR_STACK_UNDERFLOW;
    *val_ptr = sam_s0[--sam_sp];
    return SAM_ERROR_OK;
}

int sam_push_stack(sam_word_t val)
{
    if (sam_sp >= sam_ssize)
        return SAM_ERROR_STACK_OVERFLOW;
    sam_s0[sam_sp] = val;
    sam_sp++;
    return SAM_ERROR_OK;
}


// Initialise VM state.
int sam_init(sam_word_t *s0, sam_uword_t ssize, sam_uword_t sp)
{
    if ((sam_s0 = s0) == NULL)
        return -1;
    sam_ssize = ssize;
    sam_sp = sp;

    return 0;
}
