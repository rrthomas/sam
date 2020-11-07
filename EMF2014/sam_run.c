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
static sam_word_t sam_do(sam_uword_t pc);

static sam_word_t sam_rec(sam_uword_t cond, sam_uword_t then, sam_uword_t rec1, sam_uword_t rec2)
{
    sam_word_t error = 0;
    THROW_IF_ERROR(sam_do(cond));
    sam_word_t flag;
    POP(&flag);
    if (flag)
        THROW_IF_ERROR(sam_do(then));
    else {
        THROW_IF_ERROR(sam_do(rec1));
        sam_rec(cond, then, rec1, rec2);
        THROW_IF_ERROR(sam_do(rec2));
    }
 error:
    return error;
}

static sam_word_t sam_do(sam_uword_t pc)
{
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
            PUSH(operand);
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
                POP(&a);
                PUSH(~a);
            }
            break;
        case SAM_INSN_AND:
#ifdef SAM_DEBUG
            fprintf(stderr, "AND\n");
#endif
            {
                sam_word_t a, b;
                POP(&a);
                POP(&b);
                PUSH(a & b);
            }
            break;
        case SAM_INSN_OR:
#ifdef SAM_DEBUG
            fprintf(stderr, "OR\n");
#endif
            {
                sam_word_t a, b;
                POP(&a);
                POP(&b);
                PUSH(a | b);
            }
            break;
        case SAM_INSN_XOR:
#ifdef SAM_DEBUG
            fprintf(stderr, "XOR\n");
#endif
            {
                sam_word_t a, b;
                POP(&a);
                POP(&b);
                PUSH(a ^ b);
            }
            break;
        case SAM_INSN_LSH:
#ifdef SAM_DEBUG
            fprintf(stderr, "LSH\n");
#endif
            {
                sam_word_t shift, value;
                POP(&shift);
                POP(&value);
                PUSH(shift < (sam_word_t)SAM_WORD_BIT ? LSHIFT(value, shift) : 0);
            }
            break;
        case SAM_INSN_RSH:
#ifdef SAM_DEBUG
            fprintf(stderr, "RSH\n");
#endif
            {
                sam_word_t shift, value;
                POP(&shift);
                POP(&value);
                PUSH(shift < (sam_word_t)SAM_WORD_BIT ? (sam_word_t)((sam_uword_t)value >> shift) : 0);
            }
            break;
        case SAM_INSN_ARSH:
#ifdef SAM_DEBUG
            fprintf(stderr, "ARSH\n");
#endif
            {
                sam_word_t shift, value;
                POP(&shift);
                POP(&value);
                PUSH(ARSHIFT(value, shift));
            }
            break;
        case SAM_INSN_POP:
#ifdef SAM_DEBUG
            fprintf(stderr, "POP\n");
#endif
            if (sam_sp <= sam_s0)
                THROW(SAM_ERROR_STACK_UNDERFLOW);
            sam_sp--; // FIXME: cope with all stack items
            break;
        case SAM_INSN_DUP:
#ifdef SAM_DEBUG
            fprintf(stderr, "DUP\n");
#endif
            {
                sam_uword_t depth;
                POP((sam_word_t *)&depth);
                if (depth >= sam_sp - sam_s0)
                    THROW(SAM_ERROR_STACK_UNDERFLOW);
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
                POP((sam_word_t *)&depth);
                sam_word_t value;
                POP(&value);
                if (depth >= sam_sp - sam_s0)
                    THROW(SAM_ERROR_STACK_UNDERFLOW);
                else
                    sam_m0[sam_sp - (depth + 1)] = value;
            }
            break;
        case SAM_INSN_SWAP:
            // TODO
#ifdef SAM_DEBUG
            fprintf(stderr, "SWAP\n");
#endif
            {
                // FIXME: Cope with all stack items
                sam_uword_t depth;
                POP((sam_word_t *)&depth);
                if (sam_sp <= sam_s0 || depth >= sam_sp - sam_s0 - 1)
                    THROW(SAM_ERROR_STACK_UNDERFLOW);
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
                sam_uword_t depth;
                POP((sam_word_t *)&depth);
                THROW_IF_ERROR(sam_do(sam_find_code(depth)));
            }
            break;
        case SAM_INSN_IF:
#ifdef SAM_DEBUG
            fprintf(stderr, "IF\n");
#endif
            // TODO
            break;
        case SAM_INSN_LOOP:
#ifdef SAM_DEBUG
            fprintf(stderr, "LOOP\n");
#endif
            // TODO
            break;
        case SAM_INSN_REC:
#ifdef SAM_DEBUG
            fprintf(stderr, "REC\n");
#endif
            {
                sam_uword_t cond, then, rec1, rec2;
                POP((sam_word_t *)&rec2);
                POP((sam_word_t *)&rec1);
                POP((sam_word_t *)&then);
                POP((sam_word_t *)&cond);
                cond = sam_find_code(cond);
                then = sam_find_code(then);
                rec1 = sam_find_code(rec1);
                rec2 = sam_find_code(rec2);
#ifdef SAM_DEBUG
                fprintf(stderr, "cond %u then %u rec1 %u rec2 %u\n", cond, then, rec1, rec2);
#endif
                THROW_IF_ERROR(sam_rec(cond, then, rec1, rec2));
            }
            break;
        case SAM_INSN_TIMES:
#ifdef SAM_DEBUG
            fprintf(stderr, "TIMES\n");
#endif
            {
                sam_uword_t n;
                POP((sam_word_t *)&n);
                sam_uword_t code;
                POP((sam_word_t *)&code);
#ifdef SAM_DEBUG
                fprintf(stderr, "n %u code %u\n", n, code);
#endif
                sam_uword_t i;
                for (i = 0; i < n; i++)
                    THROW_IF_ERROR(sam_do(sam_find_code(code)));
            }
            break;
        case SAM_INSN_NEG:
#ifdef SAM_DEBUG
            fprintf(stderr, "NEG\n");
#endif
            {
                sam_uword_t a;
                POP((sam_word_t *)&a);
                PUSH((sam_word_t)-a);
            }
            break;
        case SAM_INSN_ADD:
#ifdef SAM_DEBUG
            fprintf(stderr, "ADD\n");
#endif
            {
                sam_uword_t a, b;
                POP((sam_word_t *)&a);
                POP((sam_word_t *)&b);
                PUSH((sam_word_t)(b + a));
            }
            break;
        case SAM_INSN_MUL:
#ifdef SAM_DEBUG
            fprintf(stderr, "MUL\n");
#endif
            {
                sam_uword_t a, b;
                POP((sam_word_t *)&a);
                POP((sam_word_t *)&b);
                PUSH((sam_word_t)(a * b));
            }
            break;
        case SAM_INSN_DIVMOD:
#ifdef SAM_DEBUG
            fprintf(stderr, "DIVMOD\n");
#endif
            {
                sam_word_t divisor, dividend;
                POP(&divisor);
                POP(&dividend);
                if (dividend == SAM_WORD_MIN && divisor == -1) {
                    PUSH(SAM_WORD_MIN);
                    PUSH(0);
                } else {
                    PUSH(DIV_CATCH_ZERO(dividend, divisor));
                    PUSH(MOD_CATCH_ZERO(dividend, divisor));
                }
            }
            break;
        case SAM_INSN_UDIVMOD:
#ifdef SAM_DEBUG
            fprintf(stderr, "UDIVMOD\n");
#endif
            {
                sam_uword_t divisor, dividend;
                POP((sam_word_t *)&divisor);
                POP((sam_word_t *)&dividend);
                PUSH(DIV_CATCH_ZERO(dividend, divisor));
                PUSH(MOD_CATCH_ZERO(dividend, divisor));
            }
            break;
        case SAM_INSN_EQ:
#ifdef SAM_DEBUG
            fprintf(stderr, "EQ\n");
#endif
            {
                sam_word_t a, b;
                POP(&a);
                POP(&b);
                PUSH(a == b);
            }
            break;
        case SAM_INSN_LT:
#ifdef SAM_DEBUG
            fprintf(stderr, "LT\n");
#endif
            {
                sam_word_t a, b;
                POP((sam_word_t *)&a);
                POP((sam_word_t *)&b);
                PUSH(b < a);
            }
            break;
        case SAM_INSN_CATCH:
#ifdef SAM_DEBUG
            fprintf(stderr, "CATCH\n");
#endif
            // TODO
            {
                /* sam_word_t *addr; */
                /* POP((sam_word_t *)&addr); */
                /* CHECK_ALIGNED(addr); */
                /* PUSH_RETURN(sam_handler_sp); */
                /* PUSH_RETURN((sam_uword_t)pc); */
                /* sam_handler_sp = sam_sp; */
                /* pc = addr; */
            }
            break;
        case SAM_INSN_THROW:
#ifdef SAM_DEBUG
            fprintf(stderr, "THROW\n");
#endif
            {
                if (sam_sp < sam_s0 + 1)
                    error = SAM_ERROR_STACK_UNDERFLOW;
                else
                    POP(&error);
            error:
                /* if (sam_handler_sp == 0) */
                return error;
                /* // Don't push error code if the stack is full. */
                /* if (sam_sp - sam_s0 < sam_ssize) */
                /*     *(sam_sp++) = error; */
                /* sam_sp = sam_handler_sp; */
                /* sam_word_t *addr; */
                /* POP_RETURN((sam_word_t *)&addr); */
                /* POP_RETURN((sam_word_t *)&sam_handler_sp); */
                /* pc = addr; */
            }
            break;
        default:
#ifdef SAM_DEBUG
            fprintf(stderr, "ERROR_INVALID_OPCODE\n");
#endif
            THROW(SAM_ERROR_INVALID_OPCODE);
            break;
        case SAM_INSN_TRAP:
#ifdef SAM_DEBUG
            fprintf(stderr, "TRAP\n");
#endif
            THROW_IF_ERROR(trap((sam_uword_t)operand));
            break;
        }

    }
}

sam_word_t sam_run(void)
{
    return sam_do(1);
}
