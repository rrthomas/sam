// Manipulate the stack.
//
// (c) Reuben Thomas 1994-2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sam.h"
#include "sam_opcodes.h"

#include "private.h"


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
    sam_word_t old_val = s->s0[addr];
    if ((old_val & SAM_STACK_TAG_MASK) == SAM_STACK_TAG) {
        sam_stack_t *inner_s = (sam_stack_t *)(old_val & ~SAM_STACK_TAG_MASK);
        if (inner_s->type == SAM_ARRAY_STACK)
            sam_stack_unref(inner_s);
    }
    if ((val & SAM_STACK_TAG_MASK) == SAM_STACK_TAG) {
        sam_stack_t *inner_s = (sam_stack_t *)(val & ~SAM_STACK_TAG_MASK);
        if (inner_s->type == SAM_ARRAY_STACK)
            sam_stack_ref(inner_s);
    }
    s->s0[addr] = val;
    return SAM_ERROR_OK;
}

// Move `size` words anywhere within allocated stack memory.
// The blocks may overlap.
static sam_word_t move_n(sam_stack_t *s, sam_uword_t dst, sam_uword_t src, sam_uword_t size)
{
    if (size > s->ssize || src > s->ssize - size || dst > s->ssize - size)
        return SAM_ERROR_INVALID_ADDRESS;
    memmove(&s->s0[dst], &s->s0[src], size * sizeof(sam_word_t));
    return SAM_ERROR_OK;
}

// Move the stack item at `addr` to the top of `s`.
int sam_stack_extract(sam_stack_t *s, sam_uword_t addr)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t opcode;
    HALT_IF_ERROR(sam_stack_peek(s, addr, &opcode));
    HALT_IF_ERROR(move_n(s, addr, addr + 1, s->sp - addr));
    HALT_IF_ERROR(sam_stack_poke(s, s->sp - 1, opcode));
error:
    return error;
}

// Move the top item of `s` to `addr`.
int sam_stack_insert(sam_stack_t *s, sam_uword_t addr)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t opcode;
    HALT_IF_ERROR(sam_stack_peek(s, s->sp - 1, &opcode));
    HALT_IF_ERROR(move_n(s, addr + 1, addr, s->sp - addr));
    HALT_IF_ERROR(sam_stack_poke(s, addr, opcode));
error:
    return error;
}

// s gives the stack to consider.
// If n < 0, return offset of stack item n from top; otherwise,
// check n is the address of a stack item, and return it.
int sam_stack_item(sam_stack_t *s, sam_word_t n, sam_uword_t *addr)
{
    if (n < 0)
        n = s->sp + n;
    if (n >= s->sp)
        return SAM_ERROR_STACK_OVERFLOW;
    sam_word_t error = SAM_ERROR_OK;
    sam_uword_t p = n, inst;
    HALT_IF_ERROR(sam_stack_peek(s, p, &inst));
    *addr = p;
 error:
    return error;
}

int sam_stack_pop(sam_stack_t *s, sam_word_t *val_ptr)
{
    sam_word_t error = SAM_ERROR_OK;
    if (s->sp == 0)
        return SAM_ERROR_STACK_UNDERFLOW;
    HALT_IF_ERROR(sam_stack_peek(s, s->sp - 1, (sam_uword_t *)val_ptr));
    HALT_IF_ERROR(sam_stack_poke(s, s->sp - 1, SAM_INSTS_TAG));
    s->sp--;
 error:
    return error;
}

int sam_stack_push(sam_stack_t *s, sam_word_t val)
{
    sam_word_t error = SAM_ERROR_OK;
    if (s->sp >= s->ssize) {
        sam_uword_t new_size = s->ssize * 2;
        s->s0 = realloc(s->s0, new_size * sizeof(sam_uword_t));
        if (s->s0 == NULL)
            HALT(SAM_ERROR_NO_MEMORY);
        memset(s->s0 + s->ssize, 0, s->ssize * sizeof(sam_uword_t));
        s->ssize = new_size;
    }
    HALT_IF_ERROR(sam_stack_poke(s, s->sp++, val));
 error:
    return error;
}

int sam_push_ref(sam_stack_t *s, void *ptr)
{
    return sam_stack_push(s, SAM_STACK_TAG | ((sam_uword_t)ptr));
}

int sam_push_int(sam_stack_t *s, sam_uword_t val)
{
    return sam_stack_push(s, SAM_INT_TAG | (val << SAM_INT_SHIFT));
}

int sam_push_float(sam_stack_t *s, sam_float_t n)
{
    sam_uword_t operand = *(sam_uword_t *)&n;
    return sam_stack_push(s, SAM_FLOAT_TAG | ((operand & ~SAM_FLOAT_TAG_MASK) << SAM_FLOAT_SHIFT));
}

int sam_push_atom(sam_stack_t *s, sam_uword_t atom_type, sam_uword_t operand)
{
    sam_uword_t atom = SAM_ATOM_TAG | (atom_type << SAM_ATOM_TYPE_SHIFT) | (operand << SAM_ATOM_SHIFT);
    return sam_stack_push(s, atom);
}

int sam_push_trap(sam_stack_t *s, sam_uword_t function)
{
    // FIXME: error if function code is too large
    return sam_stack_push(s, SAM_TRAP_TAG | (function << SAM_TRAP_FUNCTION_SHIFT));
}

int sam_push_insts(sam_stack_t *s, sam_uword_t insts)
{
    // FIXME: error if too many bits
    return sam_stack_push(s, SAM_INSTS_TAG | (insts << SAM_INSTS_SHIFT));
}

void sam_stack_ref(sam_stack_t *s) {
    s->nrefs++;
}

void sam_stack_unref(sam_stack_t *s) {
    assert(s->nrefs > 0);
    if (--s->nrefs == 0)
        sam_stack_free(s); // FIXME: check return value
}

int sam_stack_free(sam_stack_t *s) {
    sam_word_t error = SAM_ERROR_OK;

    // Unref the contents of the stack.
    while (s->sp > 0) {
        sam_word_t val;
        HALT_IF_ERROR(sam_stack_pop(s, &val));
    }

    // Free the stack's storage.
    free(s->s0);
    free(s);

error:
    return error;
}

int sam_stack_new(sam_state_t *state, unsigned type, sam_stack_t **new_stack)
{
    // FIXME: validate type
    sam_stack_t *s = calloc(sizeof(sam_stack_t), 1);
    s->type = type;
    if (s != NULL) {
        s->ssize = 1;
        s->s0 = calloc(sizeof(sam_word_t), s->ssize);
        if (s->s0 == NULL)
            return SAM_ERROR_NO_MEMORY;
    }
    *new_stack = s;
    return SAM_ERROR_OK;
}

int sam_stack_copy(sam_state_t *state, sam_stack_t *s, sam_stack_t **new_stack)
{
    int error = SAM_ERROR_OK;
    HALT_IF_ERROR(sam_stack_new(state, s->type, new_stack));
    
   // Copy the contents of the stack.
    for (sam_uword_t pos = 0; pos < s->sp; pos++) {
        sam_uword_t val;
        HALT_IF_ERROR(sam_stack_peek(s, pos, &val));
        HALT_IF_ERROR(sam_stack_push(*new_stack, val));
    }

 error:
    return error;
}
