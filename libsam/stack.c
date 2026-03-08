// SAM stacks.
//
// (c) Reuben Thomas 1994-2026
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


int sam_stack_from_blob(sam_blob_t *blob, sam_stack_t **s)
{
    sam_word_t error = SAM_ERROR_OK;
    EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, *s);
error:
    return error;
}

int sam_stack_new(sam_blob_t **new_stack)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_blob_t *blob;
    HALT_IF_ERROR(sam_blob_new(SAM_BLOB_STACK, sizeof(sam_stack_t), &blob));
    sam_stack_t *s;
    EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    s->size = 1;
    s->data = calloc(s->size, sizeof(sam_word_t));
    if (s->data == NULL) {
        free(blob);
        HALT(SAM_ERROR_NO_MEMORY);
    }
    *new_stack = blob;
error:
    return error;
}

int sam_stack_copy(sam_blob_t *stack, sam_blob_t **new_stack)
{
    int error = SAM_ERROR_OK;
    sam_stack_t *s;
    EXTRACT_BLOB(stack, SAM_BLOB_STACK, sam_stack_t, s);
    HALT_IF_ERROR(sam_stack_new(new_stack));

    // Copy the contents of the stack.
    for (sam_uword_t pos = 0; pos < s->sp; pos++) {
        sam_uword_t val;
        HALT_IF_ERROR(sam_stack_peek(stack, pos, &val));
        HALT_IF_ERROR(sam_stack_push(*new_stack, val));
    }

 error:
    return error;
}

int sam_stack_peek(sam_blob_t *blob, sam_uword_t addr, sam_uword_t *val)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_stack_t *s;
    EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    if (addr >= s->sp)
        return SAM_ERROR_INVALID_ADDRESS;
    *val = s->data[addr];

error:
    return error;
}

int sam_stack_poke(sam_blob_t *blob, sam_uword_t addr, sam_uword_t val)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_stack_t *s;
    EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    if (addr >= s->size)
        return SAM_ERROR_INVALID_ADDRESS;
    s->data[addr] = val;
error:
    return error;
}

// Move `size` words anywhere within allocated stack memory.
// The blocks may overlap.
static sam_word_t move_n(sam_stack_t *s, sam_uword_t dst, sam_uword_t src, sam_uword_t size)
{
    if (size > s->size || src > s->size - size || dst > s->size - size)
        return SAM_ERROR_INVALID_ADDRESS;
    memmove(&s->data[dst], &s->data[src], size * sizeof(sam_word_t));
    return SAM_ERROR_OK;
}

// Move the stack item at `addr` to the top of `s`.
int sam_stack_extract(sam_blob_t *blob, sam_uword_t addr)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_stack_t *s;
    EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    sam_uword_t opcode;
    HALT_IF_ERROR(sam_stack_peek(blob, addr, &opcode));
    HALT_IF_ERROR(move_n(s, addr, addr + 1, s->sp - addr));
    HALT_IF_ERROR(sam_stack_poke(blob, s->sp - 1, opcode));
error:
    return error;
}

// Move the top item of `s` to `addr`.
int sam_stack_insert(sam_blob_t *blob, sam_uword_t addr)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_stack_t *s;
    EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    sam_uword_t opcode;
    HALT_IF_ERROR(sam_stack_peek(blob, s->sp - 1, &opcode));
    HALT_IF_ERROR(move_n(s, addr + 1, addr, s->sp - addr));
    HALT_IF_ERROR(sam_stack_poke(blob, addr, opcode));
error:
    return error;
}

// s gives the stack to consider.
// If n < 0, return offset of stack item n from top; otherwise,
// check n is the address of a stack item, and return it.
int sam_stack_item(sam_blob_t *blob, sam_word_t n, sam_uword_t *addr)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_stack_t *s;
    EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    if (n < 0)
        n = s->sp + n;
    if ((sam_uword_t)n >= s->sp)
        return SAM_ERROR_STACK_OVERFLOW;
    sam_uword_t p = n, inst;
    HALT_IF_ERROR(sam_stack_peek(blob, p, &inst));
    *addr = p;
 error:
    return error;
}

int sam_stack_pop(sam_blob_t *blob, sam_word_t *val_ptr)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_stack_t *s;
    EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    if (s->sp == 0)
        return SAM_ERROR_STACK_UNDERFLOW;
    HALT_IF_ERROR(sam_stack_peek(blob, s->sp - 1, (sam_uword_t *)val_ptr));
    s->sp--;
 error:
    return error;
}

int sam_stack_shift(sam_blob_t *blob, sam_word_t *val_ptr)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_stack_t *s;
    EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    if (s->sp == 0)
        return SAM_ERROR_STACK_UNDERFLOW;
    HALT_IF_ERROR(sam_stack_peek(blob, 0, (sam_uword_t *)val_ptr));
    memmove(s->data, s->data + 1, s->sp * sizeof(sam_uword_t));
    s->sp--;
 error:
    return error;
}

static int stack_maybe_grow(sam_stack_t *s)
{
    sam_word_t error = SAM_ERROR_OK;
    if (s->sp >= s->size) {
        sam_uword_t new_size = s->size * 2;
        s->data = realloc(s->data, new_size * sizeof(sam_uword_t));
        if (s->data == NULL)
            HALT(SAM_ERROR_NO_MEMORY);
        memset(s->data + s->size, 0, s->size * sizeof(sam_uword_t));
        s->size = new_size;
    }
error:
    return error;
}

int sam_stack_push(sam_blob_t *blob, sam_word_t val)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_stack_t *s;
    EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    HALT_IF_ERROR(stack_maybe_grow(s));
    HALT_IF_ERROR(sam_stack_poke(blob, s->sp++, val));
 error:
    return error;
}

int sam_stack_prepend(sam_blob_t *blob, sam_word_t val)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_stack_t *s;
    EXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    HALT_IF_ERROR(stack_maybe_grow(s));
    memmove(s->data + 1, s->data, s->sp * sizeof(sam_uword_t));
    HALT_IF_ERROR(sam_stack_poke(blob, 0, val));
    s->sp++;
 error:
    return error;
}

int sam_push_ref(sam_blob_t *blob, void *ptr)
{
    return sam_stack_push(blob, SAM_BLOB_TAG | ((sam_uword_t)ptr));
}

int sam_push_int(sam_blob_t *blob, sam_uword_t val)
{
    return sam_stack_push(blob, SAM_INT_TAG | (val << SAM_INT_SHIFT));
}

int sam_push_float(sam_blob_t *blob, sam_float_t n)
{
    sam_uword_t operand = *(sam_uword_t *)&n;
    return sam_stack_push(blob, SAM_FLOAT_TAG | ((operand & ~SAM_FLOAT_TAG_MASK) << SAM_FLOAT_SHIFT));
}

int sam_push_atom(sam_blob_t *blob, sam_uword_t atom_type, sam_uword_t operand)
{
    sam_uword_t atom = SAM_ATOM_TAG | (atom_type << SAM_ATOM_TYPE_SHIFT) | (operand << SAM_ATOM_SHIFT);
    return sam_stack_push(blob, atom);
}

int sam_push_trap(sam_blob_t *blob, sam_uword_t function)
{
    // FIXME: error if function code is too large
    return sam_stack_push(blob, SAM_TRAP_TAG | (function << SAM_TRAP_FUNCTION_SHIFT));
}

int sam_push_insts(sam_blob_t *blob, sam_uword_t insts)
{
    // FIXME: error if too many bits
    return sam_stack_push(blob, SAM_INSTS_TAG | (insts << SAM_INSTS_SHIFT));
}
