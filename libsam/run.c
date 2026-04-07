// The interpreter main loop.
//
// (c) Reuben Thomas 1994-2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <stdlib.h>

#ifdef SAM_DEBUG
#include <stdio.h>
#endif

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_sdl.h"

#include "private.h"
#include "run.h"
#include "traps_basic.h"
#include "traps_math.h"
#include "traps_graphics.h"
#include "traps_input.h"
#include "traps_string.h"
#include "traps_audio.h"

// Instruction constants
const sam_word_t SAM_FLOAT_TAG = 0x0;
const sam_word_t SAM_FLOAT_TAG_MASK = 0x1;
const int SAM_FLOAT_SHIFT = 0;

const sam_word_t SAM_INT_TAG = 0x1;
const sam_word_t SAM_INT_TAG_MASK = 0x3;
const int SAM_INT_SHIFT = 2;

const sam_word_t SAM_BLOB_TAG = 0x3;
const sam_word_t SAM_BLOB_TAG_MASK = 0x7;
const int SAM_BLOB_SHIFT = 3;

const sam_word_t SAM_ATOM_TAG = 0x7;
const sam_word_t SAM_ATOM_TAG_MASK = 0xf;
const sam_word_t SAM_ATOM_TYPE_MASK = 0x78;
const int SAM_ATOM_TYPE_SHIFT = 4;
const int SAM_ATOM_SHIFT = 8;

const sam_word_t SAM_TRAP_TAG = 0xf;
const sam_word_t SAM_TRAP_TAG_MASK = 0x1f;
const int SAM_TRAP_FUNCTION_SHIFT = 5;

const sam_word_t SAM_INST_SET_MASK = 0x1c0;
const int SAM_INSTS_SHIFT = 9;
const sam_word_t SAM_INSTS_TAG = 0x1f;
const sam_word_t SAM_INSTS_TAG_MASK = 0x3f;
const int SAM_INST_SET_SHIFT = 6;
const sam_word_t SAM_INST_MASK = 0x1f;
const int SAM_ONE_INST_SHIFT = 5;

const sam_word_t SAM_TRAP_BASE_MASK = ~0xff;

// Trap dispatcher
static sam_word_t sam_trap(sam_state_t *state, sam_uword_t function)
{
    switch (function & SAM_TRAP_BASE_MASK) {
    case SAM_TRAP_BASIC_BASE:
        return sam_basic_trap(state, function);
    case SAM_TRAP_MATH_BASE:
        return sam_math_trap(state, function);
    case SAM_TRAP_GRAPHICS_BASE:
        return sam_graphics_trap(state, function);
    case SAM_TRAP_STRING_BASE:
        return sam_string_trap(state, function);
    case SAM_TRAP_INPUT_BASE:
        return sam_input_trap(state, function);
    case SAM_TRAP_AUDIO_BASE:
        return sam_audio_trap(state, function);
    default:
        return SAM_ERROR_INVALID_TRAP;
    }
}

// Execution function
sam_word_t sam_run(sam_state_t *state)
{
#define s ((sam_array_t *)state->s0->data)
    sam_word_t error = SAM_ERROR_OK;
    CHECK_BLOB(state->s0, SAM_BLOB_ARRAY);

    for (;;) {
        sam_array_t *p0;
        EXTRACT_BLOB(state->p0, SAM_BLOB_ARRAY, sam_array_t, p0);
        if (state->pc == p0->sp) {
#ifdef SAM_DEBUG
            debug("sam_run: p0 = %p, pc = %u, s0 = %p, sp = %u\n", state->p0, state->pc, s, s->sp);
            sam_print_working_stack(state->s0);
#endif
            debug("done\n");
            DONE;
            continue;
        }

        sam_uword_t ir;
        HALT_IF_ERROR(sam_array_peek(state->p0, state->pc++, &ir));
#ifdef SAM_DEBUG
        debug("sam_run: p0 = %p, pc = %u, s0 = %p, sp = %u, ir = %x\n", state->p0, state->pc, s, s->sp, ir);
        sam_print_working_stack(state->s0);
#endif

        if ((ir & SAM_BLOB_TAG_MASK) == SAM_BLOB_TAG) {
#ifdef SAM_DEBUG
            debug("blob\n");
#endif
            PUSH_WORD(ir); // Push the same blob on the stack
        } else if ((ir & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
#ifdef SAM_DEBUG
            debug("int\n");
#endif
            PUSH_WORD(ir);
        } else if ((ir & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
#ifdef SAM_DEBUG
            debug("float\n");
#endif
            PUSH_WORD(ir);
        } else if ((ir & SAM_ATOM_TAG_MASK) == SAM_ATOM_TAG) {
            sam_word_t atom_type = (ir & SAM_ATOM_TYPE_MASK) >> SAM_ATOM_TYPE_SHIFT;
            switch (atom_type) {
            case SAM_ATOM_NULL:
#ifdef SAM_DEBUG
                debug("null\n");
#endif
                PUSH_WORD(ir);
                break;
            case SAM_ATOM_BOOL:
#ifdef SAM_DEBUG
                debug("bool\n");
#endif
                PUSH_WORD(ir);
                break;
            default:
                HALT(SAM_ERROR_INVALID_ATOM_TYPE);
            }
        } else if ((ir & SAM_TRAP_TAG_MASK) == SAM_TRAP_TAG) {
            sam_uword_t function = ir >> SAM_TRAP_FUNCTION_SHIFT;
#ifdef SAM_DEBUG
            debug("trap %s\n", trap_name(function));
#endif
            HALT_IF_ERROR(sam_trap(state, function));
        } else if ((ir & SAM_INSTS_TAG_MASK) == SAM_INSTS_TAG) {
            for (sam_uword_t opcodes = (sam_uword_t)ir >> SAM_INSTS_SHIFT; opcodes != 0; ) {
                sam_word_t opcode = opcodes & SAM_INST_MASK;
#ifdef SAM_DEBUG
                debug("%s\n", inst_name(opcode));
#endif
                switch (opcode) {
                case INST_NOP:
                    break;
                case INST_NEW:
                    {
                        sam_blob_t *stack;
                        HALT_IF_ERROR(sam_array_new(&stack));
                        HALT_IF_ERROR(sam_push_blob(state->s0, stack));
                    }
                    break;
                case INST_S0:
                    HALT_IF_ERROR(sam_push_blob(state->s0, state->s0));
                    break;
                case INST_DROP:
                    if (s->sp < 1)
                        HALT(SAM_ERROR_ARRAY_UNDERFLOW);
                    s->sp -= 1;
                    break;
                case INST_GET:
                    {
                        sam_blob_t *blob;
                        POP_REF(blob);
                        switch (blob->type) {
                        case SAM_BLOB_ARRAY:
                            {
                                sam_word_t pos;
                                POP_INT(pos);
                                sam_uword_t addr, item;
                                HALT_IF_ERROR(sam_array_item(blob, pos, &addr));
                                HALT_IF_ERROR(sam_array_peek(blob, addr, &item));
                                HALT_IF_ERROR(sam_array_push(state->s0, item));
                            }
                            break;
                        case SAM_BLOB_MAP:
                            {
                                sam_word_t key, val;
                                POP_WORD(&key);
                                HALT_IF_ERROR(sam_map_get(blob, key, &val));
                                HALT_IF_ERROR(sam_array_push(state->s0, val));
                            }
                            break;
                        }
                    }
                    break;
                case INST_SET:
                    {
                        sam_blob_t *blob;
                        POP_REF(blob);
                        switch (blob->type) {
                        case SAM_BLOB_ARRAY:
                            {
                                sam_word_t pos, val;
                                POP_INT(pos);
                                sam_uword_t dest;
                                HALT_IF_ERROR(sam_array_item(blob, pos, &dest));
                                POP_WORD(&val);
                                HALT_IF_ERROR(sam_array_poke(blob, dest, val));
                            }
                            break;
                        case SAM_BLOB_MAP:
                            {
                                sam_word_t key, val;
                                POP_WORD(&key);
                                POP_WORD(&val);
                                HALT_IF_ERROR(sam_map_set(blob, key, val));
                            }
                            break;
                        }
                    }
                    break;
                case INST_EXTRACT:
                    {
                        sam_blob_t *blob;
                        POP_REF(blob);
                        sam_word_t pos;
                        POP_INT(pos);
                        sam_uword_t addr;
                        HALT_IF_ERROR(sam_array_item(blob, pos, &addr));
                        HALT_IF_ERROR(sam_array_extract(blob, addr));
                    }
                    break;
                case INST_INSERT:
                    {
                        sam_blob_t *blob;
                        POP_REF(blob);
                        sam_word_t pos;
                        POP_INT(pos);
                        sam_uword_t addr;
                        HALT_IF_ERROR(sam_array_item(blob, pos, &addr));
                        HALT_IF_ERROR(sam_array_insert(blob, addr));
                    }
                    break;
                case INST_POP:
                    {
                        sam_blob_t *blob;
                        POP_REF(blob);
                        sam_array_t *stack;
                        EXTRACT_BLOB(blob, SAM_BLOB_ARRAY, sam_array_t, stack);
                        if (stack->sp < 1)
                            HALT(SAM_ERROR_ARRAY_UNDERFLOW);
                        sam_word_t val;
                        HALT_IF_ERROR(sam_array_pop(blob, &val));
                        PUSH_WORD(val);
                    }
                    break;
                case INST_SHIFT:
                    {
                        sam_blob_t *blob;
                        POP_REF(blob);
                        sam_array_t *stack;
                        EXTRACT_BLOB(blob, SAM_BLOB_ARRAY, sam_array_t, stack);
                        if (stack->sp < 1)
                            HALT(SAM_ERROR_ARRAY_UNDERFLOW);
                        sam_word_t val;
                        HALT_IF_ERROR(sam_array_shift(blob, &val));
                        PUSH_WORD(val);
                    }
                    break;
                case INST_APPEND:
                    {
                        sam_blob_t *stack;
                        POP_REF(stack);
                        sam_word_t val;
                        POP_WORD(&val);
                        HALT_IF_ERROR(sam_array_push(stack, val));
                    }
                    break;
                case INST_PREPEND:
                    {
                        sam_blob_t *stack;
                        POP_REF(stack);
                        sam_word_t val;
                        POP_WORD(&val);
                        HALT_IF_ERROR(sam_array_prepend(stack, val));
                    }
                    break;
                case INST_GO:
                    {
                        sam_blob_t *code;
                        POP_REF(code);
                        GO(code);
                        opcodes = 0;
                    }
                    break;
                case INST_DO:
                    {
                        sam_blob_t *code;
                        POP_REF(code);
                        DO(code);
                        opcodes = 0;
                    }
                    break;
                case INST_CALL:
                    {
                        sam_blob_t *code, *frame;
                        POP_REF(frame);
                        POP_REF(code);
                        sam_uword_t nargs;
                        POP_UINT(nargs);
                        for (sam_uword_t i = nargs; i > 0; i--) {
                            sam_uword_t val;
                            HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - i, &val));
                            HALT_IF_ERROR(sam_array_push(frame, val));
                        }
                        sam_word_t val;
                        for (sam_uword_t i = 0; i < nargs; i++)
                            POP_WORD(&val);
                        HALT_IF_ERROR(sam_push_blob(frame, state->s0));
                        PUSH_INT(state->pc);
                        state->s0 = frame;
                        PUSH_REF(state->p0);
                        GO(code);
                        opcodes = 0;
                    }
                    break;
                case INST_IF:
                    {
                        sam_blob_t *then, *else_;
                        POP_REF(else_);
                        POP_REF(then);
                        sam_word_t flag;
                        POP_BOOL(flag);
                        sam_blob_t *addr = flag ? then : else_;
                        DO(addr);
                        opcodes = 0;
                    }
                    break;
                case INST_WHILE:
                    {
                        sam_word_t flag;
                        POP_BOOL(flag);
                        if (!flag) {
                            DONE;
                            opcodes = 0;
                        }
                    }
                    break;
                case INST_NOT:
                    {
                        sam_uword_t operand;
                        sam_word_t a;
                        HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - 1, &operand));
                        if ((operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                            POP_INT(a);
                            PUSH_INT(~a);
                        } else {
                            POP_BOOL(a);
                            PUSH_BOOL(!a);
                        }
                    }
                    break;
                case INST_AND:
                    {
                        sam_uword_t operand;
                        sam_word_t a, b;
                        HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - 1, &operand));
                        if ((operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                            POP_INT(b);
                            POP_INT(a);
                            PUSH_INT(a & b);
                        } else {
                            POP_BOOL(b);
                            POP_BOOL(a);
                            PUSH_BOOL(a & b);
                        }
                    }
                    break;
                case INST_OR:
                    {
                        sam_uword_t operand;
                        sam_word_t a, b;
                        HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - 1, &operand));
                        if ((operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                            POP_INT(b);
                            POP_INT(a);
                            PUSH_INT(a | b);
                        } else {
                            POP_BOOL(b);
                            POP_BOOL(a);
                            PUSH_BOOL(a | b);
                        }
                    }
                    break;
                case INST_XOR:
                    {
                        sam_uword_t operand;
                        sam_word_t a, b;
                        HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - 1, &operand));
                        if ((operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                            POP_INT(b);
                            POP_INT(a);
                            PUSH_INT(a ^ b);
                        } else {
                            POP_BOOL(b);
                            POP_BOOL(a);
                            PUSH_BOOL(a ^ b);
                        }
                    }
                    break;
                case INST_EQ:
                    {
                        sam_word_t x, y;
                        POP_WORD(&y);
                        POP_WORD(&x);
                        PUSH_BOOL(x == y);
                    }
                    break;
                case INST_LT:
                    {
                        sam_uword_t operand;
                        HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - 1, &operand));
                        if ((operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                            sam_word_t a, b;
                            POP_INT(b);
                            POP_INT(a);
                            PUSH_BOOL(a < b);
                        } else if ((operand & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
                            sam_float_t a, b;
                            POP_FLOAT(b);
                            POP_FLOAT(a);
                            PUSH_BOOL(a < b);
                        } else
                            HALT(SAM_ERROR_WRONG_TYPE);
                    }
                    break;
                case INST_NEG:
                    {
                        sam_uword_t operand;
                        HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - 1, &operand));
                        if ((operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                            sam_uword_t a;
                            POP_UINT(a);
                            PUSH_INT(-a);
                        } else if ((operand & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
                            sam_float_t a;
                            POP_FLOAT(a);
                            PUSH_FLOAT(-a);
                        } else
                            HALT(SAM_ERROR_WRONG_TYPE);
                    }
                    break;
                case INST_ADD:
                    {
                        sam_uword_t operand;
                        HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - 1, &operand));
                        if ((operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                            sam_uword_t a, b;
                            POP_UINT(b);
                            POP_UINT(a);
                            PUSH_INT((sam_word_t)(a + b));
                        } else if ((operand & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
                            sam_float_t a, b;
                            POP_FLOAT(b);
                            POP_FLOAT(a);
                            PUSH_FLOAT(a + b);
                        } else
                            HALT(SAM_ERROR_WRONG_TYPE);
                    }
                    break;
                case INST_MUL:
                    {
                        sam_uword_t operand;
                        HALT_IF_ERROR(sam_array_peek(state->s0, s->sp - 1, &operand));
                        if ((operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                            sam_uword_t a, b;
                            POP_UINT(b);
                            POP_UINT(a);
                            PUSH_INT((sam_word_t)(a * b));
                        } else if ((operand & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
                            sam_float_t a, b;
                            POP_FLOAT(b);
                            POP_FLOAT(a);
                            PUSH_FLOAT(a * b);
                        } else
                            HALT(SAM_ERROR_WRONG_TYPE);
                    }
                    break;
                case INST_0:
                    PUSH_INT(0);
                    break;
                case INST_1:
                    PUSH_INT(1);
                    break;
                case INST_MINUS_1:
                    PUSH_INT(-1);
                    break;
                case INST_2:
                    PUSH_INT(2);
                    break;
                case INST_MINUS_2:
                    PUSH_INT(-2);
                    break;
                case INST_HALT:
                    HALT(SAM_ERROR_OK);
                    break;
                }

                opcodes >>= SAM_ONE_INST_SHIFT;

#ifdef SAM_DEBUG
                if (opcodes != 0) {
                    debug("sam_run: p0 = %p, pc = %u, s0 = %p, sp = %u, ir = %x\n", state->p0, state->pc, s, s->sp, ir);
                    sam_print_working_stack(state->s0);
                }
#endif
            }
        } else {
            abort(); // The opcodes are exhaustive
        }

        sam_sdl_process_events();
    }

error:
    return error;
}
