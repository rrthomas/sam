// SAM's basic traps.
//
// (c) Reuben Thomas 2020-2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include "sam.h"
#include "sam_opcodes.h"
#include "private.h"
#include "run.h"
#include "traps_basic.h"

sam_word_t sam_basic_trap(sam_state_t *state, sam_uword_t function)
{
    sam_stack_t *s = state->stack;
    int error = SAM_ERROR_OK;
    
    switch (function) {
    case TRAP_BASIC_S0:
        HALT_IF_ERROR(sam_push_ref(s, s));
        break;
    case TRAP_BASIC_QUOTE:
        {
            sam_uword_t ir;
            HALT_IF_ERROR(sam_stack_peek(state->pc0, state->pc++, &ir));
            PUSH_WORD(ir);
        }
        break;
    case TRAP_BASIC_NEW:
        {
            sam_stack_t *stack;
            HALT_IF_ERROR(sam_stack_new(state, SAM_ARRAY_STACK, &stack));
            HALT_IF_ERROR(sam_push_ref(s, stack));
        }
    break;
    case TRAP_BASIC_COPY:
        {
            sam_word_t item;
            HALT_IF_ERROR(sam_stack_pop_unsafe(s, &item));
            CHECK_TYPE(item, SAM_STACK_TAG_MASK, SAM_STACK_TAG);
            sam_stack_t *stack = (sam_stack_t *)(item & ~SAM_STACK_TAG_MASK);
            sam_stack_t *new_stack;
            HALT_IF_ERROR(sam_stack_copy(state, stack, &new_stack));
            PUSH_REF(new_stack); // Now the copied stack will be unreferenced.
        }
    break;
    case TRAP_BASIC_LSH:
        {
            sam_word_t shift, value;
            POP_INT(shift);
            POP_INT(value);
            PUSH_INT(shift < (sam_word_t)SAM_WORD_BIT ? LSHIFT(value, shift) : 0);
        }
        break;
    case TRAP_BASIC_RSH:
        {
            sam_word_t shift, value;
            POP_INT(shift);
            POP_INT(value);
            PUSH_INT(shift < (sam_word_t)SAM_WORD_BIT ? (sam_word_t)((sam_uword_t)value >> shift) : 0);
        }
        break;
    case TRAP_BASIC_ARSH:
        {
            sam_word_t shift, value;
            POP_INT(shift);
            POP_INT(value);
            PUSH_INT(ARSHIFT(value, shift));
        }
        break;
    }
 error:
    return error;
}

char *sam_basic_trap_name(sam_word_t function)
{
    switch (function) {
    case TRAP_BASIC_S0:
        return "S0";
    case TRAP_BASIC_QUOTE:
        return "QUOTE";
    case TRAP_BASIC_NEW:
        return "NEW";
    case TRAP_BASIC_COPY:
        return "COPY";
    case TRAP_BASIC_LSH:
        return "LSH";
    case TRAP_BASIC_RSH:
        return "RSH";
    case TRAP_BASIC_ARSH:
        return "ARSH";
    default:
        return NULL;
    }
}
