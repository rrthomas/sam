// Allocate storage for the registers and memory.
//
// (c) Reuben Thomas 1994-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <stdlib.h>
#include <string.h>

#include "sam.h"
#include "sam_opcodes.h"

#include "private.h"


// VM registers

#define R(reg, type) \
    type sam_##reg;
#include "sam_registers.h"
#undef R
static sam_word_t *sam_s0;
sam_uword_t sam_program_len;


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

// Push the stack item at `addr` of size `size` to the top of the stack.
int sam_stack_get(sam_uword_t addr, sam_uword_t size)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t opcode;
    HALT_IF_ERROR(sam_stack_peek(addr, &opcode));
    opcode &= SAM_TAG_MASK;
    if (opcode == SAM_TAG_ARRAY)
      PUSH_PTR(addr);
    else {
      sam_uword_t i;
      for (i = 0; i < size; i++) {
        sam_uword_t temp;
        HALT_IF_ERROR(sam_stack_peek(addr + i, &temp));
        PUSH_WORD(temp);
      }
    }
error:
    return error;
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

// Overwrite one block with another of possibly unequal size in the stack.
// The blocks must not overlap.
int sam_stack_set(sam_uword_t addr1, sam_uword_t size1, sam_uword_t addr2, sam_uword_t size2)
{
    if (size1 > sam_sp || size2 > sam_sp || addr1 > sam_sp - size1 || addr2 > sam_sp - size2)
        return SAM_ERROR_STACK_OVERFLOW;
    if (size1 > size2)
        move_n(addr1 + size2, addr1 + size1, sam_sp - (addr1 + size1));
    move_n(addr2, addr1, size1);
    if (size1 < size2)
        move_n(addr1 + size1, addr1 + size2, sam_sp - (addr1 + size1));
    return SAM_ERROR_OK;
}

static int stack_item_bottom(sam_uword_t s0, sam_uword_t sp,sam_uword_t n, sam_uword_t *addr, sam_uword_t *size)
{
    if (n >= s0 + sp)
        return SAM_ERROR_STACK_OVERFLOW;
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t p = s0 + n, inst;
    HALT_IF_ERROR(sam_stack_peek(p++, &inst));
    sam_word_t operand = ARSHIFT(inst, SAM_OPERAND_SHIFT);
    // If instruction is a STACK with positive argument, skip to matching
    // STACK.
    if ((inst & SAM_TAG_MASK) == SAM_TAG_ARRAY && operand > 0) {
        p += operand;
        sam_uword_t opcode2;
        HALT_IF_ERROR(sam_stack_peek(p++, &opcode2));
        if ((opcode2 & SAM_TAG_MASK) != SAM_TAG_ARRAY)
            return SAM_ERROR_BAD_BRACKET;
    } else if ((inst & (SAM_TAG_MASK | SAM_BIATOM_TAG_MASK | SAM_BIATOM_TYPE_MASK)) == (SAM_TAG_BIATOM | (SAM_BIATOM_FIRST << SAM_BIATOM_TAG_SHIFT) | (SAM_BIATOM_FLOAT << SAM_BIATOM_TYPE_SHIFT))) {
        sam_uword_t opcode2;
        HALT_IF_ERROR(sam_stack_peek(p++, &opcode2));
        CHECK_BIATOM_SECOND(opcode2, SAM_BIATOM_FLOAT);
    }
    *addr = s0 + n;
    *size = p - (s0 + n);
 error:
    return error;
}

static int stack_item_top(sam_uword_t s0, sam_uword_t sp, sam_uword_t n, sam_uword_t *addr, sam_uword_t *size)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t p = s0 + sp;
    sam_uword_t i, last_sp;
    for (i = 0; i < n; i++) {
        last_sp = p;
        if (p == s0)
            return SAM_ERROR_STACK_UNDERFLOW;
        sam_uword_t inst;
        HALT_IF_ERROR(sam_stack_peek(--p, &inst));
        sam_word_t operand = ARSHIFT(inst, SAM_OPERAND_SHIFT);
        // If instruction is a STACK with negative argument, skip to matching
        // STACK.
        if ((inst & SAM_TAG_MASK) == SAM_TAG_ARRAY && operand < 0) {
            p += operand;
            sam_uword_t opcode2;
            HALT_IF_ERROR(sam_stack_peek(p, &opcode2));
            if ((opcode2 & SAM_TAG_MASK) != SAM_TAG_ARRAY)
                return SAM_ERROR_BAD_BRACKET;
        } else if ((inst & (SAM_TAG_MASK | SAM_BIATOM_TAG_MASK | SAM_BIATOM_TYPE_MASK)) == (SAM_TAG_BIATOM | (SAM_BIATOM_SECOND << SAM_BIATOM_TAG_SHIFT) | (SAM_BIATOM_FLOAT << SAM_BIATOM_TYPE_SHIFT))) {
            sam_uword_t opcode2;
            HALT_IF_ERROR(sam_stack_peek(--p, &opcode2));
            CHECK_BIATOM_FIRST(opcode2, SAM_BIATOM_FLOAT);
        }
    }
    *addr = p;
    *size = last_sp - p;
 error:
    return error;
}

// s0 and sp give the address of the stack to consider.
// If n < 0, return offset of stack item n from top; otherwise,
// check n is the address of a stack item, and return it.
int sam_stack_item(sam_uword_t s0, sam_uword_t sp, sam_word_t n, sam_uword_t *addr, sam_uword_t *size)
{
    if (n >= 0)
        return stack_item_bottom(s0, sp, n, addr, size);
    else
        return stack_item_top(s0, sp, -n, addr, size);
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
    if (sam_sp >= sam_ssize) {
        sam_ssize = sam_sp + 65536;
        sam_s0 = realloc(sam_s0, sam_ssize);
        if (sam_s0 == NULL)
            HALT(SAM_ERROR_NO_MEMORY);
    }
    HALT_IF_ERROR(sam_stack_poke(sam_sp++, val));
 error:
    return error;
}

int sam_pop(sam_word_t **ptr, sam_uword_t *size)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t addr;
    HALT_IF_ERROR(sam_stack_item(0, sam_sp, -1, &addr, size));
    *ptr = malloc(*size * sizeof(sam_word_t));
    if (*ptr == NULL)
        HALT(SAM_ERROR_NO_MEMORY);
    for (sam_uword_t i = 0; i < *size; i++) {
        sam_uword_t val;
        HALT_IF_ERROR(sam_stack_peek(addr + i, &val));
        (*ptr)[i] = val;
    }
    sam_sp -= *size;
error:
    return error;
}

int sam_push(sam_word_t *ptr, sam_uword_t size)
{
    sam_word_t error = SAM_ERROR_OK;
    for (sam_uword_t i = 0; i < size; i++, sam_sp++)
        HALT_IF_ERROR(sam_stack_poke(sam_sp, ptr[i]));
error:
    return error;
}

// Initialise VM state.
int sam_init(sam_word_t *m0, sam_uword_t msize, sam_uword_t sp)
{
    if ((sam_s0 = m0) == NULL)
        return -1;
    sam_ssize = msize;
    sam_program_len = sam_sp = sp;

    return 0;
}
