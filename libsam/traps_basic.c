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
#define s ((sam_stack_t *)state->s0->data)
    sam_word_t error = SAM_ERROR_OK;
    CHECK_BLOB(state->s0, SAM_BLOB_STACK);
    
    switch (function) {
    case TRAP_BASIC_S0:
        HALT_IF_ERROR(sam_push_ref(state->s0, state->s0));
        break;
    case TRAP_BASIC_SIZE:
        {
            sam_blob_t *blob;
            POP_REF(blob);
            sam_stack_t *stack;
            EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, stack);
            PUSH_INT(stack->sp);
        }
        break;
    case TRAP_BASIC_QUOTE:
        {
            sam_uword_t ir;
            HALT_IF_ERROR(sam_stack_peek(state->p0, state->pc++, &ir));
            PUSH_WORD(ir);
        }
    break;
    case TRAP_BASIC_PREPEND:
        {
            sam_blob_t *stack;
            POP_REF(stack);
            sam_word_t val;
            POP_WORD(&val);
            HALT_IF_ERROR(sam_stack_prepend(stack, val));
        }
        break;
    case TRAP_BASIC_SHIFT:
        {
            sam_blob_t *blob;
            POP_REF(blob);
            sam_stack_t *stack;
            EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, stack);
            if (stack->sp < 1)
                HALT(SAM_ERROR_STACK_UNDERFLOW);
            sam_word_t val;
            HALT_IF_ERROR(sam_stack_shift(blob, &val));
            PUSH_WORD(val);
        }
        break;
    case TRAP_BASIC_NEW:
        {
            sam_blob_t *stack;
            HALT_IF_ERROR(sam_stack_new(&stack));
            HALT_IF_ERROR(sam_push_ref(state->s0, stack));
        }
        break;
    case TRAP_BASIC_COPY:
        {
            sam_blob_t *blob;
            POP_REF(blob);
            sam_blob_t *new_blob;
            switch (blob->type) {
                case SAM_BLOB_STACK:
                    HALT_IF_ERROR(sam_stack_copy(blob, &new_blob));
                    break;
                case SAM_BLOB_MAP:
                    HALT_IF_ERROR(sam_map_copy(blob, &new_blob));
                    break;
            }
            PUSH_REF(new_blob);
        }
        break;
    case TRAP_BASIC_RET:
        {
            sam_word_t item;
            HALT_IF_ERROR(sam_stack_pop(state->s0, &item));
            sam_blob_t *frame, *old_stack_blob = state->s0;
            sam_stack_t *old_stack;
            EXTRACT_BLOB(old_stack_blob, SAM_BLOB_STACK, sam_stack_t, old_stack);
            DONE;
            POP_REF(frame);
            state->s0 = frame;
            PUSH_WORD(item);
            // Wipe the stack slot for the return value.
            HALT_IF_ERROR(sam_stack_poke(old_stack_blob, old_stack->sp, SAM_INSTS_TAG));
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
    case TRAP_BASIC_ITER:
        break;
    case TRAP_BASIC_NEXT:
        break;
    case TRAP_BASIC_NEW_MAP:
        {
            sam_blob_t *map;
            HALT_IF_ERROR(sam_map_new(&map));
            HALT_IF_ERROR(sam_push_ref(state->s0, map));
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
    case TRAP_BASIC_SIZE:
        return "SIZE";
    case TRAP_BASIC_QUOTE:
        return "QUOTE";
    case TRAP_BASIC_PREPEND:
        return "PREPEND";
    case TRAP_BASIC_SHIFT:
        return "SHIFT";
    case TRAP_BASIC_NEW:
        return "NEW";
    case TRAP_BASIC_COPY:
        return "COPY";
    case TRAP_BASIC_RET:
        return "RET";
    case TRAP_BASIC_LSH:
        return "LSH";
    case TRAP_BASIC_RSH:
        return "RSH";
    case TRAP_BASIC_ARSH:
        return "ARSH";
    case TRAP_BASIC_NEW_MAP:
        return "NEW_MAP";
    case TRAP_BASIC_ITER:
        return "ITER";
    case TRAP_BASIC_NEXT:
        return "NEXT";
    default:
        return NULL;
    }
}
