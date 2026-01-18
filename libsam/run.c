// The interpreter main loop.
//
// (c) Reuben Thomas 1994-2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <math.h>
#include <stdlib.h>

#ifdef SAM_DEBUG
#include <stdio.h>
#endif

#include "sam.h"
#include "sam_opcodes.h"
#include "private.h"
#include "run.h"
#include "traps_math.h"
#include "traps_graphics.h"

// Instruction constants
const sam_word_t SAM_FLOAT_TAG = 0x0;
const sam_word_t SAM_FLOAT_TAG_MASK = 0x1;
const int SAM_FLOAT_SHIFT = 0;

#if SIZE_MAX == 4294967295ULL
const sam_word_t SAM_STACK_TAG = 0x1;
const sam_word_t SAM_STACK_TAG_MASK = 0x3;
const int SAM_STACK_SHIFT = 2;

const sam_word_t SAM_INT_TAG = 0x3;
const sam_word_t SAM_INT_TAG_MASK = 0x7;
const int SAM_INT_SHIFT = 3;

const sam_word_t SAM_INST_SET_MASK = 0x40;
const int SAM_INSTS_SHIFT = 7;
#elif SIZE_MAX == 18446744073709551615ULL
const sam_word_t SAM_INT_TAG = 0x1;
const sam_word_t SAM_INT_TAG_MASK = 0x3;
const int SAM_INT_SHIFT = 2;

const sam_word_t SAM_STACK_TAG = 0x3;
const sam_word_t SAM_STACK_TAG_MASK = 0x7;
const int SAM_STACK_SHIFT = 3;

const sam_word_t SAM_INST_SET_MASK = 0x1c0;
const int SAM_INSTS_SHIFT = 9;
#else
#error "SAM needs 4- or 8-byte size_t"
#endif

const sam_word_t SAM_ATOM_TAG = 0x7;
const sam_word_t SAM_ATOM_TAG_MASK = 0xf;
const sam_word_t SAM_ATOM_TYPE_MASK = 0x78;
const int SAM_ATOM_TYPE_SHIFT = 4;
const int SAM_ATOM_SHIFT = 8;

const sam_word_t SAM_TRAP_TAG = 0xf;
const sam_word_t SAM_TRAP_TAG_MASK = 0x1f;
const int SAM_TRAP_FUNCTION_SHIFT = 5;

const sam_word_t SAM_INSTS_TAG = 0x1f;
const sam_word_t SAM_INSTS_TAG_MASK = 0x3f;
const int SAM_INST_SET_SHIFT = 6;
const sam_word_t SAM_INST_MASK = 0x1f;
const int SAM_INST_SHIFT = 5;

const sam_word_t SAM_TRAP_BASE_MASK = ~0xff;

// Execution macros
#define GO(addr)                                \
    do {                                        \
        sam_frame_t *inner_f = sam_frame_new(); \
        if (inner_f == NULL)                    \
            HALT(SAM_ERROR_NO_MEMORY);          \
        inner_f->code = (sam_stack_t *)addr;    \
        inner_f->stack = f->stack;              \
        inner_f->pc = 0;                        \
        f = inner_f;                            \
    } while (0)

#define DO(addr)                                \
    do {                                        \
        PUSH_PTR(f);                            \
        GO(addr);                               \
    } while (0)


// FIXME: reference-count frames
#define RET                                     \
    do {                                        \
        sam_frame_t *old_f = f;                 \
        POP_PTR(f);                             \
        free(old_f);                            \
    } while (0)

// Division macros
#define DIV_CATCH_ZERO(a, b) ((b) == 0 ? 0 : (a) / (b))
#define MOD_CATCH_ZERO(a, b) ((b) == 0 ? (a) : (a) % (b))

// Trap dispatcher
static sam_word_t sam_trap(sam_frame_t *f, sam_uword_t function)
{
    switch (function & SAM_TRAP_BASE_MASK) {
    case SAM_TRAP_MATH_BASE:
        return sam_math_trap(f, function);
    case SAM_TRAP_GRAPHICS_BASE:
        return sam_graphics_trap(f, function);
    default:
        return SAM_ERROR_INVALID_TRAP;
    }
}

// Execution function
sam_word_t sam_run(sam_state_t *state)
{
    sam_frame_t *f = state->root_frame;
    sam_word_t error = SAM_ERROR_OK;

    for (;;) {
        while (f->pc == f->code->sp) {
#ifdef SAM_DEBUG
            debug("ket\n");
#endif
            RET;
        }

        sam_uword_t ir;
        HALT_IF_ERROR(sam_stack_peek(f->code, f->pc++, &ir));
#ifdef SAM_DEBUG
        debug("sam_run: pc0 = %p, pc = %u, sp = %u, ir = %x\n", f->code, f->pc - 1, f->stack->sp, ir);
        sam_print_working_stack(f->stack);
#endif

        if ((ir & SAM_STACK_TAG_MASK) == SAM_STACK_TAG) {
#ifdef SAM_DEBUG
            debug("ref\n");
#endif
            PUSH_WORD(ir); // Push the same ref on the stack
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
            // No atoms yet.
            switch ((ir & SAM_ATOM_TYPE_MASK) >> SAM_ATOM_TYPE_SHIFT) {}
        } else if ((ir & SAM_TRAP_TAG_MASK) == SAM_TRAP_TAG) {
            sam_uword_t function = ir >> SAM_TRAP_FUNCTION_SHIFT;
#ifdef SAM_DEBUG
            debug("trap %s\n", trap_name(function));
#endif
            HALT_IF_ERROR(sam_trap(f, function));
        } else if ((ir & SAM_INSTS_TAG_MASK) == SAM_INSTS_TAG) {
            for (sam_uword_t opcodes = (sam_uword_t)ir >> SAM_INSTS_SHIFT; opcodes != 0; ) {
                sam_word_t opcode = opcodes & SAM_INST_MASK;
#ifdef SAM_DEBUG
                debug("%s\n", inst_name(opcode));
#endif
                switch (opcode) {
                case INST_NOP:
                    break;
                case INST_POP:
                    if (f->stack->sp < 1)
                        HALT(SAM_ERROR_STACK_UNDERFLOW);
                    f->stack->sp -= 1;
                    break;
                case INST_GET:
                    {
                        sam_word_t pos;
                        POP_INT(pos);
                        sam_uword_t addr, item;
                        HALT_IF_ERROR(sam_stack_item(f->stack, pos, &addr));
                        HALT_IF_ERROR(sam_stack_peek(f->stack, addr, &item));
                        HALT_IF_ERROR(sam_stack_push(f->stack, item));
                    }
                    break;
                case INST_SET:
                    {
                        sam_word_t pos, val;
                        POP_INT(pos);
                        sam_uword_t dest;
                        POP_WORD(&val);
                        HALT_IF_ERROR(sam_stack_item(f->stack, pos, &dest));
                        HALT_IF_ERROR(sam_stack_poke(f->stack, dest, val));
                    }
                    break;
                case INST_EXTRACT:
                    {
                        sam_word_t pos;
                        POP_INT(pos);
                        sam_uword_t addr;
                        HALT_IF_ERROR(sam_stack_item(f->stack, pos, &addr));
                        HALT_IF_ERROR(sam_stack_extract(f->stack, addr));
                    }
                    break;
                case INST_INSERT:
                    {
                        sam_word_t pos;
                        POP_INT(pos);
                        sam_uword_t addr;
                        HALT_IF_ERROR(sam_stack_item(f->stack, pos, &addr));
                        HALT_IF_ERROR(sam_stack_insert(f->stack, addr));
                    }
                    break;
                case INST_IGET:
                    {
                        sam_stack_t *stack;
                        POP_PTR(stack);
                        sam_word_t pos;
                        POP_INT(pos);
                        sam_uword_t addr, item;
                        HALT_IF_ERROR(sam_stack_item(stack, pos, &addr));
                        HALT_IF_ERROR(sam_stack_peek(stack, addr, &item));
                        HALT_IF_ERROR(sam_stack_push(f->stack, item));
                    }
                    break;
                case INST_ISET:
                    {
                        sam_stack_t *stack;
                        POP_PTR(stack);
                        sam_word_t pos, val;
                        POP_INT(pos);
                        POP_WORD(&val);
                        sam_uword_t dest;
                        HALT_IF_ERROR(sam_stack_item(stack, pos, &dest));
                        HALT_IF_ERROR(sam_stack_poke(stack, dest, val));
                    }
                    break;
                case INST_GO:
                    {
                        sam_stack_t *code;
                        POP_PTR(code);
                        GO(code);
                        opcodes = 0;
                    }
                    break;
                case INST_DO:
                    {
                        sam_stack_t *code;
                        POP_PTR(code);
                        DO(code);
                        opcodes = 0;
                    }
                    break;
                case INST_IF:
                    {
                        sam_stack_t *then, *else_;
                        POP_PTR(else_);
                        POP_PTR(then);
                        sam_word_t flag;
                        POP_INT(flag);
                        sam_stack_t *addr = flag ? then : else_;
                        DO(addr);
                        opcodes = 0;
                    }
                    break;
                case INST_WHILE:
                    {
                        sam_word_t flag;
                        POP_INT(flag);
                        if (!flag) {
                            RET;
                            opcodes = 0;
                        }
                    }
                    break;
                case INST_NOT:
                    {
                        sam_word_t a;
                        POP_INT(a);
                        PUSH_INT(~a);
                    }
                    break;
                case INST_AND:
                    {
                        sam_word_t a, b;
                        POP_INT(b);
                        POP_INT(a);
                        PUSH_INT(a & b);
                    }
                    break;
                case INST_OR:
                    {
                        sam_word_t a, b;
                        POP_INT(b);
                        POP_INT(a);
                        PUSH_INT(a | b);
                    }
                    break;
                case INST_XOR:
                    {
                        sam_word_t a, b;
                        POP_INT(b);
                        POP_INT(a);
                        PUSH_INT(a ^ b);
                    }
                    break;
                case INST_LSH:
                    {
                        sam_word_t shift, value;
                        POP_INT(shift);
                        POP_INT(value);
                        PUSH_INT(shift < (sam_word_t)SAM_WORD_BIT ? LSHIFT(value, shift) : 0);
                    }
                    break;
                case INST_RSH:
                    {
                        sam_word_t shift, value;
                        POP_INT(shift);
                        POP_INT(value);
                        PUSH_INT(shift < (sam_word_t)SAM_WORD_BIT ? (sam_word_t)((sam_uword_t)value >> shift) : 0);
                    }
                    break;
                case INST_ARSH:
                    {
                        sam_word_t shift, value;
                        POP_INT(shift);
                        POP_INT(value);
                        PUSH_INT(ARSHIFT(value, shift));
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
                        HALT_IF_ERROR(sam_stack_peek(f->stack, f->stack->sp - 1, &operand));
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
                        HALT_IF_ERROR(sam_stack_peek(f->stack, f->stack->sp - 1, &operand));
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
                        HALT_IF_ERROR(sam_stack_peek(f->stack, f->stack->sp - 1, &operand));
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
                        HALT_IF_ERROR(sam_stack_peek(f->stack, f->stack->sp - 1, &operand));
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
                case INST_DIV:
                    {
                        sam_uword_t operand;
                        HALT_IF_ERROR(sam_stack_peek(f->stack, f->stack->sp - 1, &operand));
                        if ((operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                            sam_word_t divisor, dividend;
                            POP_INT(divisor);
                            POP_INT(dividend);
                            if (dividend == SAM_INT_MIN && divisor == -1) {
                                PUSH_INT(SAM_INT_MIN);
                            } else {
                                PUSH_INT(DIV_CATCH_ZERO(dividend, divisor));
                            }
                        } else if ((operand & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
                            sam_float_t divisor, dividend;
                            POP_FLOAT(divisor);
                            POP_FLOAT(dividend);
                            PUSH_FLOAT(DIV_CATCH_ZERO(dividend, divisor));
                        } else
                            HALT(SAM_ERROR_WRONG_TYPE);
                    }
                    break;
                case INST_REM:
                    {
                        sam_uword_t operand;
                        HALT_IF_ERROR(sam_stack_peek(f->stack, f->stack->sp - 1, &operand));
                        if ((operand & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
                            sam_uword_t divisor, dividend;
                            POP_UINT(divisor);
                            POP_UINT(dividend);
                            PUSH_INT(MOD_CATCH_ZERO(dividend, divisor));
                        } else if ((operand & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
                            sam_float_t divisor, dividend;
                            POP_FLOAT(divisor);
                            POP_FLOAT(dividend);
                            PUSH_FLOAT(divisor == 0 ? dividend : fmodf(divisor, dividend));
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
                    if (f->stack->sp < 1)
                        HALT(SAM_ERROR_STACK_UNDERFLOW);
                    else {
                        sam_word_t ret;
                        POP_INT(ret);
                        HALT(SAM_ERROR_HALT | ret << SAM_RET_SHIFT);
                    }
                    break;
                }

                opcodes >>= SAM_INST_SHIFT;

#ifdef SAM_DEBUG
                if (opcodes != 0) {
                    debug("sam_run: pc = %u, sp = %u, ir = %x\n", f->pc - 1, f->stack->sp, ir);
                    sam_print_working_stack(f->stack);
                }
#endif
            }
        } else {
            abort(); // The opcodes are exhaustive
        }

        sam_graphics_process_events();
    }

error:
    return error;
}
