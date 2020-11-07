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
#endif

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"

// Division macros
#define DIV_CATCH_ZERO(a, b) ((b) == 0 ? 0 : (a) / (b))
#define MOD_CATCH_ZERO(a, b) ((b) == 0 ? (a) : (a) % (b))


// Execution function
sam_word_t sam_run(void)
{
    for (;;) {
        sam_word_t error = SAM_ERROR_OK, ir = *sam_pc++;
#ifdef SAM_DEBUG
        fprintf(stderr, "sam_run: pc = %p, ir = %x\n", sam_pc, ir);
#endif

        switch (ir & SAM_OP_MASK) {
        case SAM_OP_STACK:
            // TODO
            break;
        case SAM_OP_LIT:
#ifdef SAM_DEBUG
            fprintf(stderr, "LIT\n");
#endif
            PUSH(ARSHIFT(ir, SAM_OP_SHIFT));
            break;
        case SAM_OP_TRAP:
#ifdef SAM_DEBUG
            fprintf(stderr, "TRAP\n");
#endif
            THROW_IF_ERROR(trap((sam_uword_t)ir >> SAM_OP_SHIFT));
            break;
        case SAM_OP_INSN:
            {
                sam_uword_t opcode = (sam_uword_t)ir >> SAM_OP_SHIFT;
                switch (opcode) {
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
                    if (sam_sp == 0)
                        THROW(SAM_ERROR_STACK_UNDERFLOW);
                    sam_sp--;
                    break;
                case SAM_INSN_DUP:
#ifdef SAM_DEBUG
                    fprintf(stderr, "DUP\n");
#endif
                    {
                        sam_uword_t depth;
                        POP((sam_word_t *)&depth);
                        if (depth >= sam_sp)
                            THROW(SAM_ERROR_STACK_UNDERFLOW);
                        else
                            PUSH(sam_s0[sam_sp - (depth + 1)]);
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
                        if (depth >= sam_sp)
                            THROW(SAM_ERROR_STACK_UNDERFLOW);
                        else
                            sam_s0[sam_sp - (depth + 1)] = value;
                    }
                    break;
                case SAM_INSN_SWAP:
#ifdef SAM_DEBUG
                    fprintf(stderr, "SWAP\n");
#endif
                    {
                        sam_uword_t depth;
                        POP((sam_word_t *)&depth);
                        if (sam_sp == 0 || depth >= sam_sp - 1)
                            THROW(SAM_ERROR_STACK_UNDERFLOW);
                        else {
                            sam_word_t temp = sam_s0[sam_sp - (depth + 2)];
                            sam_s0[sam_sp - (depth + 2)] = sam_s0[sam_sp - 1];
                            sam_s0[sam_sp - 1] = temp;
                        }
                    }
                    break;
                case SAM_INSN_PUSH:
#ifdef SAM_DEBUG
                    fprintf(stderr, "PUSH (float)\n");
#endif
                    PUSH(*sam_pc++);
                    break;
                case SAM_INSN_NEG:
#ifdef SAM_DEBUG
                    fprintf(stderr, "NEG\n");
#endif
                    {
                        sam_float_t a;
                        POP((sam_word_t *)&a);
                        a = -a;
                        PUSH(*(sam_uword_t *)&a);
                    }
                    break;
                case SAM_INSN_ADD:
#ifdef SAM_DEBUG
                    fprintf(stderr, "ADD\n");
#endif
                    {
                        sam_float_t a, b;
                        POP((sam_word_t *)&a);
                        POP((sam_word_t *)&b);
                        a += b;
                        PUSH(*(sam_uword_t *)&a);
                    }
                    break;
                case SAM_INSN_MUL:
#ifdef SAM_DEBUG
                    fprintf(stderr, "MUL\n");
#endif
                    {
                        sam_float_t a, b;
                        POP((sam_word_t *)&a);
                        POP((sam_word_t *)&b);
                        a *= b;
                        PUSH(*(sam_uword_t *)&a);
                    }
                    break;
                case SAM_INSN_DIV:
#ifdef SAM_DEBUG
                    fprintf(stderr, "DIV\n");
#endif
                    {
                        sam_float_t divisor, dividend;
                        POP((sam_word_t *)&divisor);
                        POP((sam_word_t *)&dividend);
                        sam_float_t res = DIV_CATCH_ZERO(dividend, divisor);
                        PUSH(*(sam_uword_t *)&res);
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
                        sam_float_t a, b;
                        POP((sam_word_t *)&a);
                        POP((sam_word_t *)&b);
                        PUSH(b < a);
                    }
                    break;
                    /* case SAM_INSN_CATCH: */
                    /*     { */
                    /*         sam_word_t *addr; */
                    /*         POP((sam_word_t *)&addr); */
                    /*         CHECK_ALIGNED(addr); */
                    /*         PUSH_RETURN(sam_handler_sp); */
                    /*         PUSH_RETURN((sam_uword_t)sam_pc); */
                    /*         sam_handler_sp = sam_sp; */
                    /*         sam_pc = addr; */
                    /*     } */
                    /*     break; */
                case SAM_INSN_THROW:
#ifdef SAM_DEBUG
                    fprintf(stderr, "THROW\n");
#endif
                    {
                        if (sam_sp < 1)
                            error = SAM_ERROR_STACK_UNDERFLOW;
                        else
                            POP(&error);
                    error:
                        /* if (sam_handler_sp == 0) */
                        return error;
                        /* // Don't push error code if the stack is full. */
                        /* if (sam_sp < sam_ssize) */
                        /*     sam_s0[sam_sp++] = error; */
                        /* sam_sp = sam_handler_sp; */
                        /* sam_word_t *addr; */
                        /* POP_RETURN((sam_word_t *)&addr); */
                        /* POP_RETURN((sam_word_t *)&sam_handler_sp); */
                        /* sam_pc = addr; */
                    }
                    break;
                default:
#ifdef SAM_DEBUG
                    fprintf(stderr, "ERROR_INVALID_OPCODE\n");
#endif
                    THROW(SAM_ERROR_INVALID_OPCODE);
                    break;
                }
            }
            break;
        }
    }
}
