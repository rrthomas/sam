// SAM's basic traps.
//
// (c) Reuben Thomas 2020-2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "private.h"
#include "run.h"
#include "traps_basic.h"

// Division macros
#define DIV_CATCH_ZERO(a, b) ((b) == 0 ? 0 : (a) / (b))
#define MOD_CATCH_ZERO(a, b) ((b) == 0 ? (a) : (a) % (b))

sam_word_t sam_basic_init(void)
{
    // Initialise the random number generator
    srand48(42);

    return SAM_ERROR_OK;
}

void sam_basic_finish(void)
{
    fflush(stdout);
}

sam_word_t sam_basic_trap(sam_state_t *state, sam_uword_t function)
{
#define s ((sam_array_t *)state->s0->data)
    sam_word_t error = SAM_ERROR_OK;
    CHECK_BLOB(state->s0, SAM_BLOB_ARRAY);

    switch (function) {
    case TRAP_BASIC_SIZE:
        {
            sam_blob_t *blob;
            POP_BLOB(blob);
            sam_array_t *stack;
            EXTRACT_BLOB(blob, SAM_BLOB_ARRAY, sam_array_t, stack);
            PUSH_INT(stack->sp);
        }
        break;
    case TRAP_BASIC_QUOTE:
        {
            sam_uword_t ir;
            HALT_IF_ERROR(sam_array_peek(state->p0, state->pc++, &ir));
            PUSH_WORD(ir);
        }
    break;
    case TRAP_BASIC_COPY:
        {
            sam_blob_t *blob;
            POP_BLOB(blob);
            sam_blob_t *new_blob;
            switch (blob->type) {
                case SAM_BLOB_ARRAY:
                    HALT_IF_ERROR(sam_array_copy(blob, &new_blob));
                    break;
                case SAM_BLOB_MAP:
                    HALT_IF_ERROR(sam_map_copy(blob, &new_blob));
                    break;
            }
            PUSH_BLOB(new_blob);
        }
        break;
    case TRAP_BASIC_JUMP:
        {
            sam_word_t offset;
            POP_INT(offset);
            state->pc += offset;
        }
        break;
    case TRAP_BASIC_JUMP_IF_FALSE:
        {
            sam_word_t offset;
            POP_INT(offset);
            sam_word_t flag;
            POP_BOOL(flag);
            if (!flag)
                state->pc += offset;
        }
        break;
    case TRAP_BASIC_RET:
        {
            sam_word_t item;
            HALT_IF_ERROR(sam_array_pop(state->s0, &item));
            sam_blob_t *frame, *old_stack_blob = state->s0;
            sam_array_t *old_stack;
            EXTRACT_BLOB(old_stack_blob, SAM_BLOB_ARRAY, sam_array_t, old_stack);
            PEEK_BLOB(frame, 0);
            PEEK_BLOB(state->p0, 1);
            state->s0 = frame;
            POP_UINT(state->pc);
            PUSH_WORD(item);
            // Wipe the stack slot for the return value.
            HALT_IF_ERROR(sam_array_poke(old_stack_blob, old_stack->sp, SAM_INSTS_TAG));
        }
        break;
    case TRAP_BASIC_NEW_CLOSURE:
        {
            sam_blob_t *context, *code;
            HALT_IF_ERROR(sam_array_new(&context));
            POP_BLOB(code);
            sam_uword_t nitems;
            POP_UINT(nitems);
            for (sam_uword_t i = nitems; i > 0; i--) {
                sam_uword_t val;
                HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - i, &val));
                HALT_IF_ERROR(sam_array_push(context, val));
            }
            sam_word_t val;
            for (sam_uword_t i = 0; i < nitems; i++)
                POP_WORD(&val);
            sam_blob_t *cl_blob;
            HALT_IF_ERROR(sam_closure_new(&cl_blob, code, context));
            sam_word_t inst;
            HALT_IF_ERROR(sam_make_inst_blob(&inst, cl_blob));
            HALT_IF_ERROR(sam_array_push(state->s0, inst));
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
    case TRAP_BASIC_DIV:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - 1, &operand));
            if ((sam_word_t)(operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                sam_word_t divisor, dividend;
                POP_INT(divisor);
                POP_INT(dividend);
                if (dividend == SAM_INT_MIN && divisor == -1) {
                    PUSH_INT(SAM_INT_MIN);
                } else {
                    PUSH_INT(DIV_CATCH_ZERO(dividend, divisor));
                }
            } else if ((sam_word_t)(operand & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
                sam_float_t divisor, dividend;
                POP_FLOAT(divisor);
                POP_FLOAT(dividend);
                PUSH_FLOAT(DIV_CATCH_ZERO(dividend, divisor));
            } else
                HALT(SAM_ERROR_WRONG_TYPE);
        }
        break;
    case TRAP_BASIC_REM:
        {
            sam_uword_t operand;
            HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - 1, &operand));
            if ((sam_word_t)(operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                sam_uword_t divisor, dividend;
                POP_UINT(divisor);
                POP_UINT(dividend);
                PUSH_INT(MOD_CATCH_ZERO(dividend, divisor));
            } else if ((sam_word_t)(operand & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
                sam_float_t divisor, dividend;
                POP_FLOAT(divisor);
                POP_FLOAT(dividend);
                PUSH_FLOAT(divisor == 0 ? dividend : fmodf(divisor, dividend));
            } else
                HALT(SAM_ERROR_WRONG_TYPE);
        }
        break;
    case TRAP_BASIC_ITER:
        {
            sam_word_t opcode;
            POP_WORD(&opcode);

            if ((opcode & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                {
                    sam_uword_t n = opcode >> SAM_INT_SHIFT;
                    sam_blob_t *iter;
                    HALT_IF_ERROR(sam_int_iter_new(n, &iter));
                    PUSH_BLOB(iter);
                }
                break;
            } else if ((opcode & SAM_ATOM_TAG_MASK) == SAM_ATOM_TAG) {
                sam_word_t atom_type = (opcode & SAM_ATOM_TYPE_MASK) >> SAM_ATOM_TYPE_SHIFT;
                switch (atom_type) {
                case SAM_ATOM_NULL:
                    {
                        sam_blob_t *blob, *iter;
                        HALT_IF_ERROR(sam_array_new(&blob));
                        // Iterator over empty list
                        HALT_IF_ERROR(sam_array_iter_new(blob, &iter));
                        PUSH_BLOB(iter);
                    }
                    break;
                default:
                    HALT(SAM_ERROR_INVALID_ATOM_TYPE);
                    break;
                }
            } else if ((opcode & SAM_BLOB_TAG_MASK) == SAM_BLOB_TAG) {
                sam_blob_t *blob = (sam_blob_t *)(opcode & ~SAM_BLOB_TAG_MASK);
                sam_blob_t *iter;
                switch (blob->type) {
                case SAM_BLOB_ARRAY:
                    HALT_IF_ERROR(sam_array_iter_new(blob, &iter));
                    break;
                case SAM_BLOB_CLOSURE:
                    iter = blob;
                    break;
                case SAM_BLOB_MAP:
                    HALT_IF_ERROR(sam_map_iter_new(blob, &iter));
                    break;
                case SAM_BLOB_STRING:
                    HALT_IF_ERROR(sam_string_iter_new(blob, &iter));
                    break;
                default:
                    HALT(SAM_ERROR_WRONG_TYPE);
                    break;
                }
                PUSH_BLOB(iter);
            } else
                HALT(SAM_ERROR_WRONG_TYPE);
        }
        break;
    case TRAP_BASIC_NEXT:
        {
            sam_blob_t *blob;
            POP_BLOB(blob);
            if (blob->type == SAM_BLOB_CLOSURE) {
                // Call closure with no arguments
                sam_blob_t *frame;
                HALT_IF_ERROR(sam_array_new(&frame));
                sam_word_t inst;
                HALT_IF_ERROR(sam_make_inst_blob(&inst, state->s0));
                HALT_IF_ERROR(sam_array_push(frame, inst));
                HALT_IF_ERROR(sam_make_inst_blob(&inst, state->p0));
                HALT_IF_ERROR(sam_array_push(frame, inst));
                sam_closure_t *cl;
                EXTRACT_BLOB(blob, SAM_BLOB_CLOSURE, sam_closure_t, cl);
                HALT_IF_ERROR(sam_make_inst_blob(&inst, cl->context));
                HALT_IF_ERROR(sam_array_push(frame, inst));
                PUSH_INT(state->pc);
                state->s0 = frame;
                GO(cl->code);
            } else {
                sam_word_t val;
                sam_iter_t *iter;
                EXTRACT_BLOB(blob, SAM_BLOB_ITER, sam_iter_t, iter);
                iter->next(iter, &val);
                PUSH_WORD(val);
            }
        }
        break;
    case TRAP_BASIC_NEW_MAP:
        {
            sam_blob_t *map;
            HALT_IF_ERROR(sam_map_new(&map));
            sam_word_t inst;
            HALT_IF_ERROR(sam_make_inst_blob(&inst, map));
            HALT_IF_ERROR(sam_array_push(state->s0, inst));
        }
        break;
    case TRAP_BASIC_DEBUG:
        {
            sam_word_t opcode;
            POP_WORD(&opcode);
            char *text = disas(opcode);
            printf("%s\n", text);
        }
        break;
    case TRAP_BASIC_LOG:
        {
            sam_blob_t *blob;
            POP_BLOB(blob);
            sam_string_t *str;
            EXTRACT_BLOB(blob, SAM_BLOB_STRING, sam_string_t, str);
            fwrite(str->str, sizeof(char), str->len, stderr);
        }
        break;
    case TRAP_BASIC_SEED:
        {
            sam_word_t w;
            POP_WORD(&w);
            srand48((long)w);
        }
        break;
    case TRAP_BASIC_RANDOM:
        PUSH_FLOAT(drand48());
    }
error:
    return error;
}

char *sam_basic_trap_name(sam_word_t function)
{
    switch (function) {
    case TRAP_BASIC_SIZE:
        return "SIZE";
    case TRAP_BASIC_QUOTE:
        return "QUOTE";
    case TRAP_BASIC_COPY:
        return "COPY";
    case TRAP_BASIC_JUMP:
        return "JUMP";
    case TRAP_BASIC_JUMP_IF_FALSE:
        return "JUMP_IF_FALSE";
    case TRAP_BASIC_RET:
        return "RET";
    case TRAP_BASIC_NEW_CLOSURE:
        return "NEW_CLOSURE";
    case TRAP_BASIC_LSH:
        return "LSH";
    case TRAP_BASIC_RSH:
        return "RSH";
    case TRAP_BASIC_ARSH:
        return "ARSH";
    case TRAP_BASIC_DIV:
        return "DIV";
    case TRAP_BASIC_REM:
        return "REM";
    case TRAP_BASIC_ITER:
        return "ITER";
    case TRAP_BASIC_NEXT:
        return "NEXT";
    case TRAP_BASIC_NEW_MAP:
        return "NEW_MAP";
    case TRAP_BASIC_DEBUG:
        return "DEBUG";
    case TRAP_BASIC_LOG:
        return "LOG";
    case TRAP_BASIC_SEED:
        return "SEED";
    case TRAP_BASIC_RANDOM:
        return "RANDOM";
    default:
        return NULL;
    }
}
