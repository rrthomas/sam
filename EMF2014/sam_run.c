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

#include "minmax.h"

// Division macros
#define DIV_CATCH_ZERO(a, b) ((b) == 0 ? 0 : (a) / (b))
#define MOD_CATCH_ZERO(a, b) ((b) == 0 ? (a) : (a) % (b))


// Swap `size` words anywhere within allocated stack memory.
// The blocks must not overlap.
static sam_word_t swap_n(sam_uword_t addr1, sam_uword_t addr2, sam_uword_t size)
{
    sam_uword_t i;
    if (size > sam_ssize || addr1 > sam_ssize - size || addr2 > sam_ssize - size)
        return SAM_ERROR_STACK_OVERFLOW;
    if ((addr1 >= addr2 && addr1 - addr2 < size) ||
        (addr1 < addr2 && addr2 - addr1 < size))
        return SAM_ERROR_INVALID_SWAP;
    for (i = 0; i < size; i++) {
        sam_uword_t temp;
        temp = sam_s0[addr1 + i];
        sam_s0[addr1 + i] = sam_s0[addr2 + i];
        sam_s0[addr2 + i] = temp;
    }
    return SAM_ERROR_OK;
}

// Move `size` words anywhere within allocated stack memory.
// The blocks may overlap.
static sam_word_t move_n(sam_uword_t dst, sam_uword_t src, sam_uword_t size)
{
    if (size > sam_ssize || src > sam_ssize - size || dst > sam_ssize - size)
        return SAM_ERROR_STACK_OVERFLOW;
    memmove(&sam_s0[dst], &sam_s0[src], size * sizeof(sam_uword_t));
    return SAM_ERROR_OK;
}

static sam_word_t roll_left(sam_uword_t addr)
{
    sam_uword_t i, temp;
    temp = sam_s0[sam_sp];
    for (i = sam_sp; i > addr; i--)
        sam_s0[i] = sam_s0[i - 1];
    sam_s0[addr] = temp;
}

static sam_word_t roll_right(sam_uword_t addr)
{
    sam_uword_t i, temp;
    temp = sam_s0[addr];
    for (i = addr; i < sam_sp; i++)
        sam_s0[i] = sam_s0[i + 1];
    sam_s0[sam_sp] = temp;
}

// Swap two blocks of possibly unequal size in the stack.
// The blocks must not overlap; from < to.
static sam_word_t swap_compact(sam_uword_t from, sam_uword_t from_size, sam_uword_t to, sam_uword_t to_size)
{
    sam_uword_t small_size = MIN(from_size, to_size);
    sam_uword_t big_size = MAX(from_size, to_size);
    swap_n(from, to, small_size);
    sam_uword_t i;
    for (i = 0; i < big_size - small_size; i++)
        (from_size < to_size ? roll_left : roll_right)(from + (big_size - small_size));
}

// Swap two blocks of possibly unequal size in the stack.
// The blocks must not overlap.
// Uses an extra (big_size - small_size) words of stack space.
static sam_word_t swap_fast(sam_uword_t addr1, sam_uword_t size1, sam_uword_t addr2, sam_uword_t size2)
{
    sam_uword_t from, from_size, to, to_size;
    if (addr1 < addr2) {
        from = addr1;
        from_size = size1;
        to = addr2;
        to_size = size2;
    } else {
        from = addr2;
        from_size = size2;
        to = addr1;
        to_size = size1;
    }

    if (from + from_size > to)
        return SAM_ERROR_INVALID_SWAP;

    if (from_size < to_size) {
        move_n(from + to_size, from + from_size, sam_sp - (from + from_size));
        swap_n(from, to + (to_size - from_size), from_size);
        move_n(from + from_size, sam_sp, to_size - from_size);
   } else if (from_size > to_size) {
        swap_n(from, to, to_size);
        move_n(sam_sp, from + to_size, from_size - to_size);
        move_n(from + to_size, from + from_size, sam_sp - (from + to_size));
    } else
        swap_n(to, from, from_size);
    return SAM_ERROR_OK;
}

// Swap two blocks of possibly unequal size in the stack.
// The blocks must not overlap.
static sam_word_t swap(sam_uword_t addr1, sam_uword_t size1, sam_uword_t addr2, sam_uword_t size2)
{
    sam_uword_t big_size = MAX(size1, size2);
    sam_uword_t small_size = MIN(size1, size2);
    if (size1 > sam_sp || size2 > sam_sp || addr1 > sam_sp - size1 || addr2 > sam_sp - size2)
        return SAM_ERROR_STACK_OVERFLOW;
    if (sam_sp - small_size > sam_ssize - big_size)
        return swap_compact(addr1, size1, addr2, size2);
    else
        return swap_fast(addr1, size1, addr2, size2);
}

// Execution function
sam_word_t sam_run(void)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_word_t pc0, pc;
    RET;

    for (;;) {
        sam_uword_t ir = sam_s0[pc++]; // FIXME: range-check all accesses
        sam_word_t operand = ARSHIFT((sam_word_t)ir, SAM_OP_SHIFT);
#ifdef SAM_DEBUG
        fprintf(stderr, "sam_do: pc = %u, sp = %u, ir = %x\n", pc - 1, sam_sp, ir);
        sam_print_stack();
#endif

        switch (ir & SAM_OP_MASK) {
        case SAM_INSN_NOP:
#ifdef SAM_DEBUG
            fprintf(stderr, "NOP\n");
#endif
            break;
        case SAM_INSN_INT:
#ifdef SAM_DEBUG
            fprintf(stderr, "INT\n");
#endif
            PUSH_WORD(ir);
            break;
        case SAM_INSN_FLOAT:
#ifdef SAM_DEBUG
            fprintf(stderr, "FLOAT\n");
#endif
            {
                sam_uword_t operand2 = sam_s0[pc++];
                if ((operand2 & SAM_OP_MASK) != SAM_INSN__FLOAT)
                    HALT(SAM_ERROR_UNPAIRED_FLOAT);
                PUSH_WORD(ir);
                PUSH_WORD(operand2);
            }
            break;
        case SAM_INSN__FLOAT:
#ifdef SAM_DEBUG
            fprintf(stderr, "_FLOAT\n");
#endif
            HALT(SAM_ERROR_UNPAIRED_FLOAT);
            break;
        case SAM_INSN_I2F:
#ifdef SAM_DEBUG
            fprintf(stderr, "I2F\n");
#endif
            {
                sam_word_t i;
                POP_INT(i);
                PUSH_FLOAT((sam_float_t)i);
            }
            break;
        case SAM_INSN_F2I:
#ifdef SAM_DEBUG
            fprintf(stderr, "F2I\n");
#endif
            {
                sam_float_t f;
                POP_FLOAT(f);
                PUSH_INT((sam_word_t)f);
            }
            break;
        case SAM_INSN_PUSH:
#ifdef SAM_DEBUG
            fprintf(stderr, "PUSH\n");
#endif
            {
                sam_uword_t operand2 = sam_s0[pc++];
                if ((operand2 & SAM_OP_MASK) != SAM_INSN__PUSH)
                    HALT(SAM_ERROR_UNPAIRED_PUSH);
                PUSH_WORD((ir & ~SAM_OP_MASK) | ((operand2 >> SAM_OP_SHIFT) & SAM_OP_MASK));
            }
            break;
        case SAM_INSN__PUSH:
#ifdef SAM_DEBUG
            fprintf(stderr, "_PUSH\n");
#endif
            HALT(SAM_ERROR_UNPAIRED_PUSH);
            break;
        case SAM_INSN_POP:
#ifdef SAM_DEBUG
            fprintf(stderr, "POP\n");
#endif
            if (sam_sp == 0)
                HALT(SAM_ERROR_STACK_UNDERFLOW);
            sam_uword_t size;
            HALT_IF_ERROR(sam_stack_item(0, &sam_sp, &size));
            break;
        case SAM_INSN_DUP:
#ifdef SAM_DEBUG
            fprintf(stderr, "DUP\n");
#endif
            {
                sam_uword_t depth;
                POP_UINT(depth);
                if (depth >= sam_sp)
                    HALT(SAM_ERROR_STACK_UNDERFLOW);
                else {
                    sam_uword_t addr, size;
                    HALT_IF_ERROR(sam_stack_item(depth, &addr, &size));
                    sam_uword_t opcode = sam_s0[addr] & SAM_OP_MASK;
                    if (opcode == SAM_INSN_BRA)
                        PUSH_LINK(addr);
                    else {
                        sam_uword_t i;
                        for (i = 0; i < size; i++)
                            PUSH_WORD(sam_s0[addr + i]);
                    }
                }
            }
            break;
        case SAM_INSN_SET:
#ifdef SAM_DEBUG
            fprintf(stderr, "SET\n");
#endif
            {
                // FIXME: cope with all stack items
                sam_uword_t depth;
                POP_UINT(depth);
                if (depth >= sam_sp)
                    HALT(SAM_ERROR_STACK_UNDERFLOW);
                sam_word_t value;
                POP_WORD(&value);
                sam_s0[sam_sp - (depth + 1)] = value;
            }
            break;
        case SAM_INSN_SWAP:
#ifdef SAM_DEBUG
            fprintf(stderr, "SWAP\n");
#endif
            {
                sam_uword_t depth;
                POP_UINT(depth);
                sam_uword_t addr1, size1, addr2, size2;
                HALT_IF_ERROR(sam_stack_item(0, &addr1, &size1));
                HALT_IF_ERROR(sam_stack_item(depth + 1, &addr2, &size2));
                swap(addr1, size1, addr2, size2);
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
            PUSH_LINK(pc - 1);
            pc += operand + 1; // Skip to next instruction
            break;
        case SAM_INSN_KET:
#ifdef SAM_DEBUG
            fprintf(stderr, "KET\n");
#endif
            RET;
            break;
        case SAM_INSN_LINK:
#ifdef SAM_DEBUG
            fprintf(stderr, "LINK\n");
#endif
            PUSH_WORD(ir); // Push the same link on the stack
            break;
        case SAM_INSN_DO:
#ifdef SAM_DEBUG
            fprintf(stderr, "DO\n");
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
            fprintf(stderr, "IF\n");
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
            fprintf(stderr, "WHILE\n");
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
            fprintf(stderr, "LOOP\n");
#endif
            pc = pc0;
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
        case SAM_INSN_EQ:
#ifdef SAM_DEBUG
            fprintf(stderr, "EQ\n");
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
            fprintf(stderr, "LT\n");
#endif
            switch (sam_s0[sam_sp - 1] & SAM_OP_MASK) {
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
            break;
        case SAM_INSN_NEG:
#ifdef SAM_DEBUG
            fprintf(stderr, "NEG\n");
#endif
            switch (sam_s0[sam_sp - 1] & SAM_OP_MASK) {
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
            break;
        case SAM_INSN_ADD:
#ifdef SAM_DEBUG
            fprintf(stderr, "ADD\n");
#endif
            switch (sam_s0[sam_sp - 1] & SAM_OP_MASK) {
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
            break;
        case SAM_INSN_MUL:
#ifdef SAM_DEBUG
            fprintf(stderr, "MUL\n");
#endif
            switch (sam_s0[sam_sp - 1] & SAM_OP_MASK) {
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
            break;
        case SAM_INSN_DIV:
#ifdef SAM_DEBUG
            fprintf(stderr, "DIV\n");
#endif
            switch (sam_s0[sam_sp - 1] & SAM_OP_MASK) {
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
            break;
        case SAM_INSN_REM:
#ifdef SAM_DEBUG
            fprintf(stderr, "REM\n");
#endif
            switch (sam_s0[sam_sp - 1] & SAM_OP_MASK) {
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
            break;
        case SAM_INSN_HALT:
#ifdef SAM_DEBUG
            fprintf(stderr, "HALT\n");
#endif
            if (sam_sp < 1)
                error = SAM_ERROR_STACK_UNDERFLOW;
            else
                POP_INT(error);
            error:
            DO(pc); // Save pc, pc0 so we can restart.
            return error;
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
            HALT_IF_ERROR(sam_trap((sam_uword_t)operand));
            break;
        }
    }
}
