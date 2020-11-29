// Allocate storage for the registers and memory.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <string.h>
#include "minmax.h"

#include "sam.h"
#include "sam_opcodes.h"

#include "sam_private.h"


// VM registers

#define R(reg, type) \
    type sam_##reg;
#include "sam_registers.h"
#undef R
static sam_word_t *sam_s0;


// Stack

int sam_stack_peek(sam_uword_t addr, sam_uword_t *val)
{
    if (addr >= sam_sp)
        return SAM_ERROR_INVALID_ADDRESS;
    *val = sam_s0[addr];
    return SAM_ERROR_OK;
}

int sam_stack_poke(sam_uword_t addr, sam_uword_t val)
{
    if (addr >= sam_sp)
        return SAM_ERROR_INVALID_ADDRESS;
    sam_s0[addr] = val;
    return SAM_ERROR_OK;
}

// Move `size` words anywhere within allocated stack memory.
// The blocks may overlap.
static sam_word_t move_n(sam_uword_t dst, sam_uword_t src, sam_uword_t size)
{
    if (size > sam_ssize || src > sam_ssize - size || dst > sam_ssize - size)
        return SAM_ERROR_INVALID_ADDRESS;
    memmove(&sam_s0[dst], &sam_s0[src], size * sizeof(sam_word_t));
    return SAM_ERROR_OK;
}

// Swap `size` words anywhere within allocated stack memory.
// The blocks must not overlap.
static sam_word_t swap_n(sam_uword_t addr1, sam_uword_t addr2, sam_uword_t size)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t i;
    if (size > sam_ssize || addr1 > sam_ssize - size || addr2 > sam_ssize - size)
        return SAM_ERROR_INVALID_ADDRESS;
    if ((addr1 >= addr2 && addr1 - addr2 < size) ||
        (addr1 < addr2 && addr2 - addr1 < size))
        return SAM_ERROR_INVALID_SWAP;
    for (i = 0; i < size; i++) {
        sam_uword_t temp1;
        HALT_IF_ERROR(sam_stack_peek(addr1 + i, &temp1));
        sam_uword_t temp2;
        HALT_IF_ERROR(sam_stack_peek(addr2 + i, &temp2));
        HALT_IF_ERROR(sam_stack_poke(addr1 + i, temp2));
        HALT_IF_ERROR(sam_stack_poke(addr2 + i, temp1));
    }
 error:
    return error;
}

static sam_word_t roll_left(sam_uword_t addr)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t temp;
    HALT_IF_ERROR(sam_stack_peek(sam_sp, &temp));
    HALT_IF_ERROR(move_n(addr + 1, addr, sam_sp - addr - 1));
    HALT_IF_ERROR(sam_stack_poke(addr, temp));
 error:
    return error;
}

static sam_word_t roll_right(sam_uword_t addr)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t temp;
    HALT_IF_ERROR(sam_stack_peek(addr, &temp));
    HALT_IF_ERROR(move_n(addr, addr + 1, sam_sp - addr - 1));
    HALT_IF_ERROR(sam_stack_poke(sam_sp, temp));
    return SAM_ERROR_OK;
 error:
    return error;
}

// Swap two blocks of possibly unequal size in the stack.
// The blocks must not overlap; from < to.
static sam_word_t swap_compact(sam_uword_t from, sam_uword_t from_size, sam_uword_t to, sam_uword_t to_size)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t small_size = MIN(from_size, to_size);
    sam_uword_t big_size = MAX(from_size, to_size);
    swap_n(from, to, small_size);
    sam_uword_t i;
    for (i = 0; i < big_size - small_size; i++)
        HALT_IF_ERROR((from_size < to_size ? roll_left : roll_right)(from + (big_size - small_size)));
 error:
    return error;
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
sam_word_t sam_stack_swap(sam_uword_t addr1, sam_uword_t size1, sam_uword_t addr2, sam_uword_t size2)
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

static int stack_item_bottom(sam_uword_t n, sam_uword_t *addr, sam_uword_t *size)
{
    if (n >= sam_sp)
        return SAM_ERROR_STACK_OVERFLOW;
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t sp = n, inst;
    HALT_IF_ERROR(sam_stack_peek(sp++, &inst));
    sam_uword_t opcode = inst & SAM_OP_MASK;
    // If instruction is a BRA, skip to matching KET.
    if (opcode == SAM_INSN_BRA) {
        sp += inst >> SAM_OP_SHIFT;
        sam_uword_t opcode2;
        HALT_IF_ERROR(sam_stack_peek(sp++, &opcode2));
        if ((opcode2 & SAM_OP_MASK) != SAM_INSN_KET)
            return SAM_ERROR_BAD_BRACKET;
    } else if (opcode == SAM_INSN_PUSH) {
        sam_uword_t opcode2;
        HALT_IF_ERROR(sam_stack_peek(sp++, &opcode2));
        if ((opcode2 & SAM_OP_MASK) != SAM_INSN__PUSH)
            return SAM_ERROR_UNPAIRED_PUSH;
    } else if (opcode == SAM_INSN__PUSH)
        return SAM_ERROR_UNPAIRED_PUSH;
    else if (opcode == SAM_INSN_FLOAT) {
        sam_uword_t opcode2;
        HALT_IF_ERROR(sam_stack_peek(sp++, &opcode2));
        if ((opcode2 & SAM_OP_MASK) != SAM_INSN__FLOAT)
            return SAM_ERROR_UNPAIRED_FLOAT;
    } else if (opcode == SAM_INSN__FLOAT)
        return SAM_ERROR_UNPAIRED_FLOAT;
    *addr = n;
    *size = sp - n;
 error:
    return error;
}

static int stack_item_top(sam_uword_t n, sam_uword_t *addr, sam_uword_t *size)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t sp = sam_sp, last_opcode = 0;
    sam_uword_t i, last_sp;
    for (i = 0; i < n; i++) {
        last_sp = sp;
        if (sp == 0)
            return SAM_ERROR_STACK_UNDERFLOW;
        sam_uword_t inst;
        HALT_IF_ERROR(sam_stack_peek(--sp, &inst));
        sam_uword_t opcode = inst & SAM_OP_MASK;
        // If instruction is a KET, skip to matching BRA.
        if (opcode == SAM_INSN_KET) {
            sp -= (inst >> SAM_OP_SHIFT) + 1;
            sam_uword_t opcode2;
            HALT_IF_ERROR(sam_stack_peek(sp, &opcode2));
            if ((opcode2 & SAM_OP_MASK) != SAM_INSN_BRA)
                return SAM_ERROR_BAD_BRACKET;
        } else if (opcode == SAM_INSN__PUSH) {
            sam_uword_t opcode2;
            HALT_IF_ERROR(sam_stack_peek(--sp, &opcode2));
            if ((opcode2 & SAM_OP_MASK) != SAM_INSN_PUSH)
                return SAM_ERROR_UNPAIRED_PUSH;
        } else if (opcode == SAM_INSN_PUSH && last_opcode != SAM_INSN__PUSH)
            return SAM_ERROR_UNPAIRED_PUSH;
        else if (opcode == SAM_INSN__FLOAT) {
            sam_uword_t opcode2;
            HALT_IF_ERROR(sam_stack_peek(--sp, &opcode2));
            if ((opcode2 & SAM_OP_MASK) != SAM_INSN_FLOAT)
                return SAM_ERROR_UNPAIRED_FLOAT;
        } else if (opcode == SAM_INSN_FLOAT && last_opcode != SAM_INSN__FLOAT)
            return SAM_ERROR_UNPAIRED_FLOAT;
        last_opcode = opcode;
    }
    *addr = sp;
    *size = last_sp - sp;
 error:
    return error;
}

// If n < 0, return offset of stack item n from top; otherwise,
// check n is the address of a stack item, and return it.
int sam_stack_item(sam_word_t n, sam_uword_t *addr, sam_uword_t *size)
{
    if (n >= 0)
        return stack_item_bottom(n, addr, size);
    else
        return stack_item_top(-n, addr, size);
}

// Given a LINK instruction, find the corresponding BRA instruction, and
// return the stack address of the first code word.
int sam_find_code(sam_uword_t code, sam_uword_t *addr)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t inst = code & SAM_OP_MASK;
    if (inst != SAM_INSN_LINK)
        return SAM_ERROR_NOT_CODE;
    do {
        *addr = code >> SAM_OP_SHIFT;
        HALT_IF_ERROR(sam_stack_peek(*addr, &code));
        inst = code & SAM_OP_MASK;
    } while (inst == SAM_INSN_LINK);
    if (inst != SAM_INSN_BRA)
        return SAM_ERROR_NOT_CODE;
    (*addr)++;
 error:
    return error;
}

int sam_pop_stack(sam_word_t *val_ptr)
{
    sam_word_t error = SAM_ERROR_OK;
    if (sam_sp == 0)
        return SAM_ERROR_STACK_UNDERFLOW;
    HALT_IF_ERROR(sam_stack_peek(sam_sp - 1, (sam_uword_t *)val_ptr));
    sam_sp--; // Decrement here so argument to sam_stack_peek is valid.
 error:
    return error;
}

int sam_push_stack(sam_word_t val)
{
    sam_word_t error = SAM_ERROR_OK;
    if (sam_sp >= sam_ssize)
        return SAM_ERROR_STACK_OVERFLOW;
    HALT_IF_ERROR(sam_stack_poke(sam_sp++, val));
 error:
    return error;
}


// Initialise VM state.
int sam_init(sam_word_t *s0, sam_uword_t ssize, sam_uword_t sp)
{
    if ((sam_s0 = s0) == NULL)
        return -1;
    sam_ssize = ssize;
    sam_sp = sp;

    return 0;
}
