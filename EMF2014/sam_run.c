// The interpreter main loop.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifdef SAM_DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif

#include <string.h>
#include <math.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"


// Division macros
#define DIV_CATCH_ZERO(a, b) ((b) == 0 ? 0 : (a) / (b))
#define MOD_CATCH_ZERO(a, b) ((b) == 0 ? (a) : (a) % (b))


// Execution function
sam_word_t sam_run(void)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_word_t pc;
    RET;

    for (;;) {
        sam_uword_t ir;
        HALT_IF_ERROR(sam_stack_peek(pc++, &ir));
        sam_word_t operand = ARSHIFT((sam_word_t)ir, SAM_OP_SHIFT);
#ifdef SAM_DEBUG
        debug("sam_do: pc = %u, sp = %u, ir = %x\n", pc - 1, sam_sp, ir);
        sam_print_working_stack();
#endif

        switch (ir & SAM_OP_MASK) {
        case SAM_INSN_NOP:
#ifdef SAM_DEBUG
            debug("NOP\n");
#endif
            break;
        case SAM_INSN_INT:
#ifdef SAM_DEBUG
            debug("INT\n");
#endif
            PUSH_WORD(ir);
            break;
        case SAM_INSN_FLOAT:
#ifdef SAM_DEBUG
            debug("FLOAT\n");
#endif
            {
                sam_uword_t operand2;
                HALT_IF_ERROR(sam_stack_peek(pc++, &operand2));
                if ((operand2 & SAM_OP_MASK) != SAM_INSN__FLOAT)
                    HALT(SAM_ERROR_UNPAIRED_FLOAT);
                PUSH_WORD(ir);
                PUSH_WORD(operand2);
            }
            break;
        case SAM_INSN__FLOAT:
#ifdef SAM_DEBUG
            debug("_FLOAT\n");
#endif
            HALT(SAM_ERROR_UNPAIRED_FLOAT);
            break;
        case SAM_INSN_I2F:
#ifdef SAM_DEBUG
            debug("I2F\n");
#endif
            {
                sam_word_t i;
                POP_INT(i);
                PUSH_FLOAT((sam_float_t)i);
            }
            break;
        case SAM_INSN_F2I:
#ifdef SAM_DEBUG
            debug("F2I\n");
#endif
            {
                sam_float_t f;
                POP_FLOAT(f);
                PUSH_INT((sam_word_t)f);
            }
            break;
        case SAM_INSN_PUSH:
#ifdef SAM_DEBUG
            debug("PUSH\n");
#endif
            {
                sam_uword_t operand2;
                HALT_IF_ERROR(sam_stack_peek(pc++, &operand2));
                if ((operand2 & SAM_OP_MASK) != SAM_INSN__PUSH)
                    HALT(SAM_ERROR_UNPAIRED_PUSH);
                PUSH_WORD((ir & ~SAM_OP_MASK) | ((operand2 >> SAM_OP_SHIFT) & SAM_OP_MASK));
            }
            break;
        case SAM_INSN__PUSH:
#ifdef SAM_DEBUG
            debug("_PUSH\n");
#endif
            HALT(SAM_ERROR_UNPAIRED_PUSH);
            break;
        case SAM_INSN_POP:
#ifdef SAM_DEBUG
            debug("POP\n");
#endif
            if (sam_sp == 0)
                HALT(SAM_ERROR_STACK_UNDERFLOW);
            sam_uword_t size;
            HALT_IF_ERROR(sam_stack_item(-1, &sam_sp, &size));
            break;
        case SAM_INSN_DUP:
#ifdef SAM_DEBUG
            debug("DUP\n");
#endif
            {
                sam_word_t pos;
                POP_INT(pos);
                sam_uword_t addr, size;
                HALT_IF_ERROR(sam_stack_item(pos, &addr, &size));
                sam_uword_t opcode;
                HALT_IF_ERROR(sam_stack_peek(addr, &opcode));
                opcode &= SAM_OP_MASK;
                if (opcode == SAM_INSN_BRA)
                    PUSH_LINK(addr);
                else {
                    sam_uword_t i;
                    for (i = 0; i < size; i++) {
                        sam_uword_t temp;
                        HALT_IF_ERROR(sam_stack_peek(addr + i, &temp));
                        PUSH_WORD(temp);
                    }
                }
            }
            break;
        case SAM_INSN_SWAP:
#ifdef SAM_DEBUG
            debug("SWAP\n");
#endif
            {
                sam_word_t pos;
                POP_INT(pos);
                sam_uword_t addr1, size1, addr2, size2;
                HALT_IF_ERROR(sam_stack_item(-1, &addr1, &size1));
                HALT_IF_ERROR(sam_stack_item(pos, &addr2, &size2));
                sam_stack_swap(addr1, size1, addr2, size2);
            }
            break;
        case SAM_INSN_IDUP:
            // TODO
#ifdef SAM_DEBUG
            debug("IDUP\n");
#endif
            break;
        case SAM_INSN_ISET:
#ifdef SAM_DEBUG
            debug("ISET\n");
#endif
            // TODO
            break;
        case SAM_INSN_BRA:
#ifdef SAM_DEBUG
            debug("BRA\n");
#endif
            PUSH_LINK(pc - 1);
            pc += operand + 1; // Skip to next instruction
            break;
        case SAM_INSN_KET:
#ifdef SAM_DEBUG
            debug("KET\n");
#endif
            RET;
            break;
        case SAM_INSN_LINK:
#ifdef SAM_DEBUG
            debug("LINK\n");
#endif
            PUSH_WORD(ir); // Push the same link on the stack
            break;
        case SAM_INSN_DO:
#ifdef SAM_DEBUG
            debug("DO\n");
#endif
            {
                sam_uword_t code;
                POP_WORD((sam_word_t *)&code);
                HALT_IF_ERROR(sam_find_code(code, &code));
                DO(code);
            }
            break;
        case SAM_INSN_IF:
#ifdef SAM_DEBUG
            debug("IF\n");
#endif
            {
                sam_uword_t then, else_;
                POP_WORD((sam_word_t *)&else_);
                POP_WORD((sam_word_t *)&then);
                HALT_IF_ERROR(sam_find_code(then, &then));
                HALT_IF_ERROR(sam_find_code(else_, &else_));
                sam_word_t flag;
                POP_INT(flag);
                DO(flag ? then : else_);
            }
            break;
        case SAM_INSN_WHILE:
#ifdef SAM_DEBUG
            debug("WHILE\n");
#endif
            {
                sam_word_t flag;
                POP_INT(flag);
                if (!flag)
                    RET;
            }
            break;
        case SAM_INSN_LOOP:
#ifdef SAM_DEBUG
            debug("LOOP\n");
#endif
            pc = sam_pc0;
            break;
        case SAM_INSN_NOT:
#ifdef SAM_DEBUG
            debug("NOT\n");
#endif
            {
                sam_word_t a;
                POP_INT(a);
                PUSH_INT(~a);
            }
            break;
        case SAM_INSN_AND:
#ifdef SAM_DEBUG
            debug("AND\n");
#endif
            {
                sam_word_t a, b;
                POP_INT(a);
                POP_INT(b);
                PUSH_INT(a & b);
            }
            break;
        case SAM_INSN_OR:
#ifdef SAM_DEBUG
            debug("OR\n");
#endif
            {
                sam_word_t a, b;
                POP_INT(a);
                POP_INT(b);
                PUSH_INT(a | b);
            }
            break;
        case SAM_INSN_XOR:
#ifdef SAM_DEBUG
            debug("XOR\n");
#endif
            {
                sam_word_t a, b;
                POP_INT(a);
                POP_INT(b);
                PUSH_INT(a ^ b);
            }
            break;
        case SAM_INSN_LSH:
#ifdef SAM_DEBUG
            debug("LSH\n");
#endif
            {
                sam_word_t shift, value;
                POP_INT(shift);
                POP_INT(value);
                PUSH_INT(shift < (sam_word_t)SAM_WORD_BIT ? LSHIFT(value, shift) : 0);
            }
            break;
        case SAM_INSN_RSH:
#ifdef SAM_DEBUG
            debug("RSH\n");
#endif
            {
                sam_word_t shift, value;
                POP_INT(shift);
                POP_INT(value);
                PUSH_INT(shift < (sam_word_t)SAM_WORD_BIT ? (sam_word_t)((sam_uword_t)value >> shift) : 0);
            }
            break;
        case SAM_INSN_ARSH:
#ifdef SAM_DEBUG
            debug("ARSH\n");
#endif
            {
                sam_word_t shift, value;
                POP_INT(shift);
                POP_INT(value);
                PUSH_INT(ARSHIFT(value, shift));
            }
            break;
        case SAM_INSN_EQ:
#ifdef SAM_DEBUG
            debug("EQ\n");
#endif
            {
                sam_word_t x;
                POP_WORD(&x);
                switch (x & SAM_OP_MASK) {
                case SAM_INSN_INT:
                case SAM_INSN_LINK:
                    {
                        sam_word_t y;
                        POP_WORD(&y);
                        PUSH_INT(x == y);
                    }
                    break;
                case SAM_INSN__PUSH:
                case SAM_INSN__FLOAT:
                    {
                        sam_word_t x2, y, y2;
                        POP_WORD(&x2);
                        if ((x & SAM_OP_MASK) == SAM_INSN__PUSH) {
                            if ((x2 & SAM_OP_MASK) != SAM_INSN_PUSH)
                                HALT(SAM_ERROR_UNPAIRED_PUSH);
                        } else if ((x & SAM_OP_MASK) == SAM_INSN__FLOAT) {
                            if ((x2 & SAM_OP_MASK) != SAM_INSN_FLOAT)
                                HALT(SAM_ERROR_UNPAIRED_FLOAT);
                        }
                        POP_WORD(&y);
                        POP_WORD(&y2);
                        PUSH_INT(x == y && x2 == y2);
                    }
                    break;
                }
            }
            break;
        case SAM_INSN_LT:
#ifdef SAM_DEBUG
            debug("LT\n");
#endif
            {
                sam_uword_t operand;
                HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
                switch (operand & SAM_OP_MASK) {
                case SAM_INSN_INT:
                    {
                        sam_word_t a, b;
                        POP_INT(a);
                        POP_INT(b);
                        PUSH_INT(a < b);
                    }
                    break;
                case SAM_INSN__FLOAT:
                    {
                        sam_float_t a, b;
                        POP_FLOAT(a);
                        POP_FLOAT(b);
                        PUSH_INT(a < b);
                    }
                    break;
                }
            }
            break;
        case SAM_INSN_NEG:
#ifdef SAM_DEBUG
            debug("NEG\n");
#endif
            {
                sam_uword_t operand;
                HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
                switch (operand & SAM_OP_MASK) {
                case SAM_INSN_INT:
                    {
                        sam_uword_t a;
                        POP_UINT(a);
                        PUSH_INT(-a);
                    }
                    break;
                case SAM_INSN__FLOAT:
                    {
                        sam_float_t a;
                        POP_FLOAT(a);
                        PUSH_FLOAT(-a);
                    }
                    break;
                }
            }
            break;
        case SAM_INSN_ADD:
#ifdef SAM_DEBUG
            debug("ADD\n");
#endif
            {
                sam_uword_t operand;
                HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
                switch (operand & SAM_OP_MASK) {
                case SAM_INSN_INT:
                    {
                        sam_uword_t a, b;
                        POP_UINT(a);
                        POP_UINT(b);
                        PUSH_INT((sam_word_t)(b + a));
                    }
                    break;
                case SAM_INSN__FLOAT:
                    {
                        sam_float_t a, b;
                        POP_FLOAT(a);
                        POP_FLOAT(b);
                        PUSH_FLOAT(a + b);
                    }
                    break;
                }
            }
            break;
        case SAM_INSN_MUL:
#ifdef SAM_DEBUG
            debug("MUL\n");
#endif
            {
                sam_uword_t operand;
                HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
                switch (operand & SAM_OP_MASK) {
                case SAM_INSN_INT:
                    {
                        sam_uword_t a, b;
                        POP_UINT(a);
                        POP_UINT(b);
                        PUSH_INT((sam_word_t)(a * b));
                    }
                    break;
                case SAM_INSN__FLOAT:
                    {
                        sam_float_t a, b;
                        POP_FLOAT(a);
                        POP_FLOAT(b);
                        PUSH_FLOAT(a * b);
                    }
                    break;
                }
            }
            break;
        case SAM_INSN_DIV:
#ifdef SAM_DEBUG
            debug("DIV\n");
#endif
            {
                sam_uword_t operand;
                HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
                switch (operand & SAM_OP_MASK) {
                case SAM_INSN_INT:
                    {
                        sam_word_t divisor, dividend;
                        POP_INT(divisor);
                        POP_INT(dividend);
                        if (dividend == SAM_WORD_MIN && divisor == -1) {
                            PUSH_INT(SAM_INT_MIN);
                        } else {
                            PUSH_INT(DIV_CATCH_ZERO(dividend, divisor));
                        }
                    }
                    break;
                case SAM_INSN__FLOAT:
                    {
                        sam_float_t divisor, dividend;
                        POP_FLOAT(divisor);
                        POP_FLOAT(dividend);
                        PUSH_FLOAT(DIV_CATCH_ZERO(divisor, dividend));
                    }
                    break;
                }
            }
            break;
        case SAM_INSN_REM:
#ifdef SAM_DEBUG
            debug("REM\n");
#endif
            {
                sam_uword_t operand;
                HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, &operand));
                switch (operand & SAM_OP_MASK) {
                case SAM_INSN_INT:
                    {
                        sam_uword_t divisor, dividend;
                        POP_UINT(divisor);
                        POP_UINT(dividend);
                        PUSH_INT(MOD_CATCH_ZERO(dividend, divisor));
                    }
                    break;
                case SAM_INSN__FLOAT:
                    {
                        sam_float_t divisor, dividend;
                        POP_FLOAT(divisor);
                        POP_FLOAT(dividend);
                        PUSH_FLOAT(divisor == 0 ? dividend : fmodf(divisor, dividend));
                    }
                    break;
                }
            }
            break;
        case SAM_INSN_HALT:
#ifdef SAM_DEBUG
            debug("HALT\n");
#endif
            if (sam_sp < 1)
                error = SAM_ERROR_STACK_UNDERFLOW;
            else
                POP_INT(error);
            error:
            DO(pc); // Save pc, sam_pc0 so we can restart.
            return error;
        default:
#ifdef SAM_DEBUG
            debug("ERROR_INVALID_OPCODE\n");
#endif
            HALT(SAM_ERROR_INVALID_OPCODE);
            break;
        case SAM_INSN_TRAP:
#ifdef SAM_DEBUG
            debug("TRAP\n");
#endif
            HALT_IF_ERROR(sam_trap((sam_uword_t)operand));
            break;
        }
    }
}
