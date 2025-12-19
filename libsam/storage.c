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

sam_uword_t sam_pc;
sam_stack_t *sam_stack;
sam_uword_t sam_program_len;


// Stack

int sam_stack_peek(sam_stack_t *s, sam_uword_t addr, sam_uword_t *val)
{
    if (addr >= s->sp)
        return SAM_ERROR_INVALID_ADDRESS;
    *val = s->s0[addr];
    return SAM_ERROR_OK;
}

int sam_stack_poke(sam_stack_t *s, sam_uword_t addr, sam_uword_t val)
{
    if (addr >= s->sp)
        return SAM_ERROR_INVALID_ADDRESS;
    s->s0[addr] = val;
    return SAM_ERROR_OK;
}

// Move `size` words anywhere within allocated stack memory.
// The blocks may overlap.
static sam_word_t move_n(sam_uword_t dst, sam_uword_t src, sam_uword_t size)
{
    if (size > sam_stack->ssize || src > sam_stack->ssize - size || dst > sam_stack->ssize - size)
        return SAM_ERROR_INVALID_ADDRESS;
    memmove(&sam_stack->s0[dst], &sam_stack->s0[src], size * sizeof(sam_word_t));
    return SAM_ERROR_OK;
}

// Move the stack item at `addr` to the top of the stack.
int sam_stack_extract(sam_uword_t addr) {
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t opcode;
    HALT_IF_ERROR(sam_stack_peek(sam_stack, addr, &opcode));
    HALT_IF_ERROR(move_n(addr, addr + 1, sam_stack->sp - addr));
    HALT_IF_ERROR(sam_stack_poke(sam_stack, sam_stack->sp - 1, opcode));
error:
    return error;
}

// Move the top stack item to `addr`.
int sam_stack_insert(sam_uword_t addr) {
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t opcode;
    HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_stack->sp - 1, &opcode));
    HALT_IF_ERROR(move_n(addr + 1, addr, sam_stack->sp - addr));
    HALT_IF_ERROR(sam_stack_poke(sam_stack, addr, opcode));
error:
    return error;
}

// Push the stack item at `addr` to the top of the stack.
int sam_stack_get(sam_uword_t addr)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t opcode;
    HALT_IF_ERROR(sam_stack_peek(sam_stack, addr, &opcode));
    opcode &= SAM_ARRAY_TAG_MASK;
    if (opcode == SAM_ARRAY_TAG)
        PUSH_PTR(addr);
    else {
        sam_uword_t temp;
        HALT_IF_ERROR(sam_stack_peek(sam_stack, addr, &temp));
        PUSH_WORD(temp);
    }
error:
    return error;
}

// s0 and sp give the address of the stack to consider.
// If n < 0, return offset of stack item n from top; otherwise,
// check n is the address of a stack item, and return it.
int sam_stack_item(sam_uword_t s0, sam_uword_t sp, sam_word_t n, sam_uword_t *addr)
{
    if (n < 0)
        n = sp + n;
    if (n >= sp)
        return SAM_ERROR_STACK_OVERFLOW;
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t p = s0 + n, inst;
    HALT_IF_ERROR(sam_stack_peek(sam_stack, p, &inst));
    *addr = p;
 error:
    return error;
}

int sam_pop_stack(sam_word_t *val_ptr)
{
    sam_word_t error = SAM_ERROR_OK;
    if (sam_stack->sp == 0)
        return SAM_ERROR_STACK_UNDERFLOW;
    HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_stack->sp - 1, (sam_uword_t *)val_ptr));
    sam_stack->sp--; // Decrement here so argument to sam_stack_peek is valid.
 error:
    return error;
}

int sam_push_stack(sam_stack_t *s, sam_word_t val)
{
    sam_word_t error = SAM_ERROR_OK;
    if (s->sp >= s->ssize) {
        s->ssize = s->sp + 65536;
        s->s0 = realloc(s->s0, s->ssize * sizeof(sam_uword_t));
        if (s->s0 == NULL)
            HALT(SAM_ERROR_NO_MEMORY);
    }
    HALT_IF_ERROR(sam_stack_poke(s, s->sp++, val));
 error:
    return error;
}

int sam_push_ref(sam_stack_t *s, sam_uword_t addr) {
    // FIXME: error if address is too large
    return sam_push_stack(s, SAM_REF_TAG | (addr << SAM_REF_SHIFT));
}

int sam_push_int(sam_stack_t *s, sam_uword_t val) {
    return sam_push_stack(s, SAM_INT_TAG | (val << SAM_INT_SHIFT));
}

int sam_push_float(sam_stack_t *s, sam_float_t n) {
    sam_uword_t operand = *(sam_uword_t *)&n;
    return sam_push_stack(s, SAM_FLOAT_TAG | ((operand & ~SAM_FLOAT_TAG_MASK) << SAM_FLOAT_SHIFT));
}

int sam_push_atom(sam_stack_t *s, sam_uword_t atom_type, sam_uword_t operand) {
    sam_uword_t atom = SAM_ATOM_TAG | (atom_type << SAM_ATOM_TYPE_SHIFT) | (operand << SAM_ATOM_SHIFT);
    return sam_push_stack(s, atom);
}

int sam_push_trap(sam_stack_t *s, sam_uword_t function) {
    // FIXME: error if function code is too large
    return sam_push_stack(s, SAM_TRAP_TAG | (function << SAM_TRAP_FUNCTION_SHIFT));
}

int sam_push_insts(sam_stack_t *s, sam_uword_t insts) {
    // FIXME: error if too many bits
    return sam_push_stack(s, SAM_INSTS_TAG | (insts << SAM_INSTS_SHIFT));
}

int sam_push_code(sam_stack_t *s, sam_word_t *ptr, sam_uword_t size) {
    sam_word_t error = SAM_ERROR_OK;
    // FIXME: error if array is too large
    HALT_IF_ERROR(sam_push_stack(s, SAM_ARRAY_TAG | (SAM_ARRAY_STACK << SAM_ARRAY_TYPE_SHIFT) | ((size + 1) << SAM_ARRAY_OFFSET_SHIFT)));
    for (sam_uword_t i = 0; i < size; i++)
        HALT_IF_ERROR(sam_push_stack(s, ptr[i]));
    HALT_IF_ERROR(sam_push_stack(s, SAM_ARRAY_TAG | (SAM_ARRAY_STACK << SAM_ARRAY_TYPE_SHIFT) | (-(size + 1) << SAM_ARRAY_OFFSET_SHIFT)));
error:
    return error;
}

sam_stack_t *sam_stack_new(void)
{
    return calloc(sizeof(sam_stack_t), 1);
}

// Initialise VM state.
int sam_init()
{
    sam_stack = sam_stack_new();

    return sam_stack != NULL;
}
