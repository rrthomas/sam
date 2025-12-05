// The interpreter main loop.
//
// (c) Reuben Thomas 1994-2025
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

// Instruction constants
const int SAM_TAG_SHIFT = 0;
const sam_word_t SAM_TAG_MASK = 0x3;

const int SAM_ATOM_TYPE_SHIFT = 2;
const int SAM_ATOM_TYPE_MASK = 0xc;

const int SAM_ARRAY_TYPE_SHIFT = 2;
const sam_word_t SAM_ARRAY_TYPE_MASK = 0xfc;

const int SAM_OPERAND_SHIFT = 8;
const sam_word_t SAM_OPERAND_MASK = ~0xff;

const int SAM_LINK_SHIFT = 2;
const sam_word_t SAM_LINK_MASK = ~0x3;

// Division macros
#define DIV_CATCH_ZERO(a, b) ((b) == 0 ? 0 : (a) / (b))
#define MOD_CATCH_ZERO(a, b) ((b) == 0 ? (a) : (a) % (b))

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

// Execution function
sam_word_t sam_run(void)
{
    sam_word_t error = SAM_ERROR_OK;

    for (;;) {
        sam_uword_t ir;
        HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_pc++, &ir));
#ifdef SAM_DEBUG
        debug("sam_run: pc = %u, sp = %u, ir = %x\n", sam_pc - 1, sam_stack->sp, ir);
        sam_print_working_stack();
#endif

        switch (ir & SAM_TAG_MASK) {
        case SAM_TAG_LINK:
#ifdef SAM_DEBUG
            debug("link\n");
#endif
            PUSH_WORD(ir); // Push the same link on the stack
            break;
        case SAM_TAG_ATOM:
            switch ((ir & SAM_ATOM_TYPE_MASK) >> SAM_ATOM_TYPE_SHIFT) {
            case SAM_ATOM_INST:
            {
                sam_word_t operand = ARSHIFT((sam_word_t)ir, SAM_OPERAND_SHIFT);
#ifdef SAM_DEBUG
                debug("%s\n", inst_name(operand));
#endif
                switch (operand) {
                case INST_NOP:
                    break;
                case INST_I2F:
                    {
                        sam_word_t i;
                        POP_INT(i);
                        PUSH_FLOAT((sam_float_t)i);
                    }
                    break;
                case INST_F2I:
                    {
                        sam_float_t f;
                        POP_FLOAT(f);
                        PUSH_INT((sam_word_t)rint(f));
                    }
                    break;
                case INST_POP:
                    if (sam_stack->sp == 0)
                        HALT(SAM_ERROR_STACK_UNDERFLOW);
                    sam_uword_t size;
                    HALT_IF_ERROR(sam_stack_item(0, sam_stack->sp, -1, &sam_stack->sp, &size));
                    break;
                case INST_GET:
                    {
                        sam_word_t pos;
                        POP_INT(pos);
                        sam_uword_t addr, size;
                        HALT_IF_ERROR(sam_stack_item(0, sam_stack->sp, pos, &addr, &size));
                        HALT_IF_ERROR(sam_stack_get(addr, size));
                    }
                    break;
                case INST_SET:
                    {
                        sam_word_t pos;
                        POP_INT(pos);
                        sam_uword_t addr1, size1, addr2, size2;
                        HALT_IF_ERROR(sam_stack_item(0, sam_stack->sp, -1, &addr1, &size1));
                        HALT_IF_ERROR(sam_stack_item(0, sam_stack->sp, pos, &addr2, &size2));
                        HALT_IF_ERROR(sam_stack_set(addr1, size1, addr2, size2));
                        sam_stack->sp -= size2;
                    }
                    break;
                case INST_IGET:
                    {
                        sam_uword_t stack_addr, stack_size;
                        POP_STACK(stack_addr, stack_size);
                        sam_word_t pos;
                        POP_INT(pos);
                        sam_uword_t addr, size;
                        HALT_IF_ERROR(sam_stack_item(stack_addr, stack_size, pos, &addr, &size));
                        HALT_IF_ERROR(sam_stack_get(addr, size));
                    }
                    break;
                case INST_ISET:
                    {
                        sam_uword_t stack_addr, stack_size;
                        POP_STACK(stack_addr, stack_size);
                        sam_word_t pos;
                        POP_INT(pos);
                        sam_uword_t addr1, size1, addr2, size2;
                        HALT_IF_ERROR(sam_stack_item(0, sam_stack->sp, -1, &addr1, &size1));
                        HALT_IF_ERROR(sam_stack_item(stack_addr, stack_size, pos, &addr2, &size2));
                        HALT_IF_ERROR(sam_stack_set(addr1, size1, addr2, size2));
                        sam_stack->sp -= size2;
                    }
                    break;
                case INST_DO:
                    {
                        sam_uword_t code;
                        POP_LINK(code);
                        DO(code);
                    }
                    break;
                case INST_IF:
                    {
                        sam_uword_t then, else_;
                        POP_LINK(else_);
                        POP_LINK(then);
                        sam_word_t flag;
                        POP_INT(flag);
                        DO(flag ? then : else_);
                    }
                    break;
                case INST_WHILE:
                    {
                        sam_word_t flag;
                        POP_INT(flag);
                        if (!flag)
                            RET;
                    }
                    break;
                case INST_LOOP: {
                    // FIXME: Make this more efficient.
                    for (sam_word_t depth = 1; depth > 0; sam_pc--) {
                        sam_uword_t inst;
                        HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_pc - 1, &inst));
                        if ((inst & SAM_TAG_MASK) == SAM_TAG_ARRAY) {
                            sam_word_t offset = ARSHIFT((sam_word_t)inst, SAM_OPERAND_SHIFT);
                            depth += offset < 0 ? 1 : -1;
                        }
                    }
                    sam_pc++;
                    break;
                }
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
                        POP_INT(a);
                        POP_INT(b);
                        PUSH_INT(a & b);
                    }
                    break;
                case INST_OR:
                    {
                        sam_word_t a, b;
                        POP_INT(a);
                        POP_INT(b);
                        PUSH_INT(a | b);
                    }
                    break;
                case INST_XOR:
                    {
                        sam_word_t a, b;
                        POP_INT(a);
                        POP_INT(b);
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
                        POP_WORD(&x);
                        POP_WORD(&y);
                        PUSH_INT(x == y);
                    }
                    break;
                case INST_LT:
                    {
                        sam_uword_t operand;
                        HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_stack->sp - 1, &operand));
                        if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_INT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_word_t a, b;
                            POP_INT(a);
                            POP_INT(b);
                            PUSH_INT(a < b);
                        } else if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_FLOAT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_float_t a, b;
                            POP_FLOAT(a);
                            POP_FLOAT(b);
                            PUSH_INT(a < b);
                        } else
                            HALT(SAM_ERROR_WRONG_TYPE);
                    }
                    break;
                case INST_NEG:
                    {
                        sam_uword_t operand;
                        HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_stack->sp - 1, &operand));
                        if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_INT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_uword_t a;
                            POP_UINT(a);
                            PUSH_INT(-a);
                        } else if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_FLOAT << SAM_ATOM_TYPE_SHIFT))) {
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
                        HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_stack->sp - 1, &operand));
                        if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_INT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_uword_t a, b;
                            POP_UINT(a);
                            POP_UINT(b);
                            PUSH_INT((sam_word_t)(b + a));
                        } else if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_FLOAT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_float_t a, b;
                            POP_FLOAT(a);
                            POP_FLOAT(b);
                            PUSH_FLOAT(a + b);
                        } else
                            HALT(SAM_ERROR_WRONG_TYPE);
                    }
                    break;
                case INST_MUL:
                    {
                        sam_uword_t operand;
                        HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_stack->sp - 1, &operand));
                        if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_INT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_uword_t a, b;
                            POP_UINT(a);
                            POP_UINT(b);
                            PUSH_INT((sam_word_t)(a * b));
                        } else if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_FLOAT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_float_t a, b;
                            POP_FLOAT(a);
                            POP_FLOAT(b);
                            PUSH_FLOAT(a * b);
                        } else
                            HALT(SAM_ERROR_WRONG_TYPE);
                    }
                    break;
                case INST_DIV:
                    {
                        sam_uword_t operand;
                        HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_stack->sp - 1, &operand));
                        if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_INT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_word_t divisor, dividend;
                            POP_INT(divisor);
                            POP_INT(dividend);
                            if (dividend == SAM_WORD_MIN && divisor == -1) {
                                PUSH_INT(SAM_INT_MIN);
                            } else {
                                PUSH_INT(DIV_CATCH_ZERO(dividend, divisor));
                            }
                        } else if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_FLOAT << SAM_ATOM_TYPE_SHIFT))) {
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
                        HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_stack->sp - 1, &operand));
                        if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_INT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_uword_t divisor, dividend;
                            POP_UINT(divisor);
                            POP_UINT(dividend);
                            PUSH_INT(MOD_CATCH_ZERO(dividend, divisor));
                        } else if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_FLOAT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_float_t divisor, dividend;
                            POP_FLOAT(divisor);
                            POP_FLOAT(dividend);
                            PUSH_FLOAT(divisor == 0 ? dividend : fmodf(divisor, dividend));
                        } else
                            HALT(SAM_ERROR_WRONG_TYPE);
                    }
                    break;
                case INST_POW:
                    {
                        sam_uword_t operand;
                        HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_stack->sp - 1, &operand));
                        if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_INT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_uword_t a, b;
                            POP_UINT(a);
                            POP_UINT(b);
                            PUSH_INT(powi(a, b));
                        } else if ((operand & (SAM_TAG_MASK | SAM_ATOM_TYPE_MASK)) == (SAM_TAG_ATOM | (SAM_ATOM_FLOAT << SAM_ATOM_TYPE_SHIFT))) {
                            sam_float_t a, b;
                            POP_FLOAT(a);
                            POP_FLOAT(b);
                            PUSH_FLOAT(powf(a, b));
                        } else
                            HALT(SAM_ERROR_WRONG_TYPE);
                    }
                    break;
                case INST_SIN:
                    {
                        sam_float_t a;
                        POP_FLOAT(a);
                        PUSH_FLOAT(sinf(a));
                    }
                    break;
                case INST_COS:
                    {
                        sam_float_t a;
                        POP_FLOAT(a);
                        PUSH_FLOAT(cosf(a));
                    }
                    break;
                case INST_DEG:
                    {
                        sam_float_t a;
                        POP_FLOAT(a);
                        PUSH_FLOAT(a * (M_1_PI * 180.0));
                    }
                    break;
                case INST_RAD:
                    {
                        sam_float_t a;
                        POP_FLOAT(a);
                        PUSH_FLOAT(a * (M_PI / 180.0));
                    }
                    break;
                case INST_HALT:
                    if (sam_stack->sp < 1)
                        HALT(SAM_ERROR_STACK_UNDERFLOW);
                    else {
                        sam_word_t ret;
                        POP_INT(ret);
                        HALT(SAM_ERROR_HALT | ret << SAM_OPERAND_SHIFT);
                    }
                    break;
                default:
                    HALT_IF_ERROR(sam_trap((sam_uword_t)operand));
                    break;
                }
            }
            break;
            case SAM_ATOM_INT:
#ifdef SAM_DEBUG
                debug("int\n");
#endif
                PUSH_WORD(ir);
                break;
            case SAM_ATOM_FLOAT:
#ifdef SAM_DEBUG
                debug("float\n");
#endif
                PUSH_WORD(ir);
                break;
            default:
#ifdef SAM_DEBUG
                debug("ERROR_INVALID_OPCODE\n");
#endif
                HALT(SAM_ERROR_INVALID_OPCODE);
                break;
            }
            break;
        case SAM_TAG_ARRAY:
        {
            sam_word_t operand = ARSHIFT((sam_word_t)ir, SAM_OPERAND_SHIFT);
            switch ((ir & SAM_ARRAY_TYPE_MASK) >> SAM_ARRAY_TYPE_SHIFT) {
            case SAM_ARRAY_STACK:
#ifdef SAM_DEBUG
                debug("%s\n", operand > 0 ? "bra" : "ket");
#endif
                if (operand > 0) {
                    PUSH_PTR(sam_pc - 1);
                    sam_pc += operand; // Skip to next instruction
                } else {
                    RET;
                }
                break;
            case SAM_ARRAY_RAW:
                // TODO
                // FALLTHROUGH
            default:
#ifdef SAM_DEBUG
                debug("ERROR_INVALID_OPCODE\n");
#endif
                HALT(SAM_ERROR_INVALID_OPCODE);
                break;
            }
        }
        break;
        default:
            abort(); // The cases are exhaustive
        }

        sam_traps_process_events();
    }

error:
    DO(sam_pc); // Save pc so we can restart.
    return error;
}
