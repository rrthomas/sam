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

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"

// Division macros
#define DIV_CATCH_ZERO(a, b) ((b) == 0 ? 0 : (a) / (b))
#define MOD_CATCH_ZERO(a, b) ((b) == 0 ? (a) : (a) % (b))


// Execution function
static sam_word_t sam_do(sam_uword_t pc)
{
    sam_word_t pc0 = pc;
    sam_word_t error = SAM_ERROR_OK;

    for (;;) {
        sam_uword_t ir = sam_m0[pc++];
        sam_word_t operand = ARSHIFT((sam_word_t)ir, SAM_OP_SHIFT);
#ifdef SAM_DEBUG
        fprintf(stderr, "sam_do: pc = %u, sp = %u, ir = %x\n", pc - 1, sam_sp, ir);
        sam_print_stack();
#endif

        switch (ir & SAM_OP_MASK) {
        case SAM_INSN_LIT:
#ifdef SAM_DEBUG
            fprintf(stderr, "LIT\n");
#endif
            PUSH(ir);
            break;
        case SAM_INSN_NOP:
#ifdef SAM_DEBUG
            fprintf(stderr, "NOP\n");
#endif
            break;
        case SAM_INSN_NOT:
#ifdef SAM_DEBUG
            fprintf(stderr, "NOT\n");
#endif
            {
                sam_word_t a;
                POP_INT(a);
                PUSH_INT(~a);
            }
            break;
        case SAM_INSN_AND:
#ifdef SAM_DEBUG
            fprintf(stderr, "AND\n");
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
            fprintf(stderr, "OR\n");
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
            fprintf(stderr, "XOR\n");
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
            fprintf(stderr, "LSH\n");
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
            fprintf(stderr, "RSH\n");
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
            fprintf(stderr, "ARSH\n");
#endif
            {
                sam_word_t shift, value;
                POP_INT(shift);
                POP_INT(value);
                PUSH_INT(ARSHIFT(value, shift));
            }
            break;
        case SAM_INSN_POP:
#ifdef SAM_DEBUG
            fprintf(stderr, "POP\n");
#endif
            if (sam_sp <= sam_s0)
                HALT(SAM_ERROR_STACK_UNDERFLOW);
            sam_sp--; // FIXME: cope with all stack items
            break;
        case SAM_INSN_DUP:
#ifdef SAM_DEBUG
            fprintf(stderr, "DUP\n");
#endif
            {
                sam_uword_t depth;
                POP_UINT(depth);
                if (depth >= sam_sp - sam_s0)
                    HALT(SAM_ERROR_STACK_UNDERFLOW);
                else // FIXME: cope with all stack items
                    PUSH(sam_m0[sam_sp - (depth + 1)]);
            }
            break;
        case SAM_INSN_SET:
#ifdef SAM_DEBUG
            fprintf(stderr, "SET\n");
#endif
            {
                sam_uword_t depth;
                POP_UINT(depth);
                sam_word_t value;
                POP(&value);
                if (depth >= sam_sp - sam_s0)
                    HALT(SAM_ERROR_STACK_UNDERFLOW);
                else
                    sam_m0[sam_sp - (depth + 1)] = value;
            }
            break;
        case SAM_INSN_SWAP:
#ifdef SAM_DEBUG
            fprintf(stderr, "SWAP\n");
#endif
            {
                // FIXME: Cope with all stack items
                sam_uword_t depth;
                POP_UINT(depth);
                if (sam_sp <= sam_s0 || depth >= sam_sp - sam_s0 - 1)
                    HALT(SAM_ERROR_STACK_UNDERFLOW);
                else {
                    sam_word_t temp = sam_m0[sam_sp - (depth + 2)];
                    sam_m0[sam_sp - (depth + 2)] = sam_m0[sam_sp - 1];
                    sam_m0[sam_sp - 1] = temp;
                }
            }
            break;
        case SAM_INSN_IDUP:
            // TODO
#ifdef SAM_DEBUG
            fprintf(stderr, "IDUP\n");
#endif
            break;
        case SAM_INSN_ISET:
#ifdef SAM_DEBUG
            fprintf(stderr, "ISET\n");
#endif
            // TODO
            break;
        case SAM_INSN_BRA:
#ifdef SAM_DEBUG
            fprintf(stderr, "BRA\n");
#endif
            PUSH((((sam_word_t)pc - 1) << SAM_OP_SHIFT) | SAM_INSN_LINK);
            pc += operand + 1; // Skip to next instruction
            break;
        case SAM_INSN_KET:
#ifdef SAM_DEBUG
            fprintf(stderr, "KET\n");
#endif
            return SAM_ERROR_OK;
        case SAM_INSN_LINK:
#ifdef SAM_DEBUG
            fprintf(stderr, "LINK\n");
#endif
            PUSH(ir); // Push the same link on the stack
            break;
        case SAM_INSN_DO:
#ifdef SAM_DEBUG
            fprintf(stderr, "DO\n");
#endif
            {
                sam_uword_t code;
                POP((sam_word_t *)&code);
                HALT_IF_ERROR(sam_find_code(code, &code));
                HALT_IF_ERROR(sam_do(code));
            }
            break;
        case SAM_INSN_IF:
#ifdef SAM_DEBUG
            fprintf(stderr, "IF\n");
#endif
            {
                sam_uword_t cond, then, else_;
                POP((sam_word_t *)&else_);
                POP((sam_word_t *)&then);
                POP((sam_word_t *)&cond);
                HALT_IF_ERROR(sam_find_code(cond, &cond));
                HALT_IF_ERROR(sam_find_code(then, &then));
                HALT_IF_ERROR(sam_find_code(else_, &else_));
#ifdef SAM_DEBUG
                fprintf(stderr, "cond %u then %u else %u\n", cond, then, else_);
#endif
                HALT_IF_ERROR(sam_do(cond));
                sam_word_t flag;
                POP_INT(flag);
                HALT_IF_ERROR(sam_do(flag ? then : else_));
            }
            break;
        case SAM_INSN_LOOP:
#ifdef SAM_DEBUG
            fprintf(stderr, "LOOP\n");
#endif
            pc = pc0;
            break;
        case SAM_INSN_WHILE:
#ifdef SAM_DEBUG
            fprintf(stderr, "WHILE\n");
#endif
            {
                sam_uword_t cond;
                POP((sam_word_t *)&cond);
                HALT_IF_ERROR(sam_find_code(cond, &cond));
#ifdef SAM_DEBUG
                fprintf(stderr, "cond %u\n", cond);
#endif
                HALT_IF_ERROR(sam_do(cond));
                sam_word_t flag;
                POP_INT(flag);
                if (!flag)
                    return SAM_ERROR_OK;
            }
            break;
        case SAM_INSN_TIMES:
#ifdef SAM_DEBUG
            fprintf(stderr, "TIMES\n");
#endif
            {
                sam_uword_t n;
                POP_UINT(n);
                sam_uword_t code;
                POP((sam_word_t *)&code);
                HALT_IF_ERROR(sam_find_code(code, &code));
#ifdef SAM_DEBUG
                fprintf(stderr, "n %u code %u\n", n, code);
#endif
                sam_uword_t i;
                for (i = 0; i < n; i++) {
                    HALT_IF_ERROR(sam_do(code));
                }
            }
            break;
        case SAM_INSN_NEG:
#ifdef SAM_DEBUG
            fprintf(stderr, "NEG\n");
#endif
            {
                sam_uword_t a;
                POP_UINT(a);
#ifdef SAM_DEBUG
                fprintf(stderr, "a = %u\n", a);
#endif
                PUSH_INT((sam_word_t)-a);
            }
            break;
        case SAM_INSN_ADD:
#ifdef SAM_DEBUG
            fprintf(stderr, "ADD\n");
#endif
            {
                sam_uword_t a, b;
                POP_UINT(a);
                POP_UINT(b);
#ifdef SAM_DEBUG
                fprintf(stderr, "a = %u, b = %u\n", a, b);
#endif
                PUSH_INT((sam_word_t)(b + a));
            }
            break;
        case SAM_INSN_MUL:
#ifdef SAM_DEBUG
            fprintf(stderr, "MUL\n");
#endif
            {
                sam_uword_t a, b;
                POP_UINT(a);
                POP_UINT(b);
#ifdef SAM_DEBUG
                fprintf(stderr, "a = %u, b = %u\n", a, b);
#endif
                PUSH_INT((sam_word_t)(a * b));
            }
            break;
        case SAM_INSN_DIVMOD:
#ifdef SAM_DEBUG
            fprintf(stderr, "DIVMOD\n");
#endif
            {
                sam_word_t divisor, dividend;
                POP_INT(divisor);
                POP_INT(dividend);
#ifdef SAM_DEBUG
                fprintf(stderr, "divisor = %d, dividend = %d\n", divisor, dividend);
#endif
                if (dividend == SAM_WORD_MIN && divisor == -1) {
                    PUSH_INT(SAM_INT_MIN);
                    PUSH_INT(0);
                } else {
                    PUSH_INT(DIV_CATCH_ZERO(dividend, divisor));
                    PUSH_INT(MOD_CATCH_ZERO(dividend, divisor));
                }
            }
            break;
        case SAM_INSN_UDIVMOD:
#ifdef SAM_DEBUG
            fprintf(stderr, "UDIVMOD\n");
#endif
            {
                sam_uword_t divisor, dividend;
                POP_UINT(divisor);
                POP_UINT(dividend);
                PUSH_INT(DIV_CATCH_ZERO(dividend, divisor));
                PUSH_INT(MOD_CATCH_ZERO(dividend, divisor));
            }
            break;
        case SAM_INSN_EQ:
#ifdef SAM_DEBUG
            fprintf(stderr, "EQ\n");
#endif
            {
                sam_word_t a, b;
                POP_INT(a);
                POP_INT(b);
                PUSH_INT(a == b);
            }
            break;
        case SAM_INSN_LT:
#ifdef SAM_DEBUG
            fprintf(stderr, "LT\n");
#endif
            {
                sam_word_t a, b;
                POP_INT(a);
                POP_INT(b);
                PUSH_INT(b < a);
            }
            break;
        case SAM_INSN_ULT:
#ifdef SAM_DEBUG
            fprintf(stderr, "ULT\n");
#endif
            {
                sam_uword_t a, b;
                POP_UINT(a);
                POP_UINT(b);
                PUSH_INT(b < a);
            }
            break;
        case SAM_INSN_HALT:
#ifdef SAM_DEBUG
            fprintf(stderr, "HALT\n");
#endif
            {
                if (sam_sp < sam_s0 + 1)
                    error = SAM_ERROR_STACK_UNDERFLOW;
                else
                    POP_INT(error);
            error:
                return error;
            }
            break;
        default:
#ifdef SAM_DEBUG
            fprintf(stderr, "ERROR_INVALID_OPCODE\n");
#endif
            HALT(SAM_ERROR_INVALID_OPCODE);
            break;
        case SAM_INSN_TRAP:
#ifdef SAM_DEBUG
            fprintf(stderr, "TRAP\n");
#endif
            HALT_IF_ERROR(trap((sam_uword_t)operand));
            break;
        }

    }
}

sam_word_t sam_run(void)
{
    return sam_do(1);
}
