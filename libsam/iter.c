// SAM iterators.
//
// (c) Reuben Thomas 2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include "sam.h"
#include "sam_opcodes.h"
#include "private.h"

int sam_iter_next(sam_iter_t *i, sam_word_t *val)
{
    return i->next(i, val);
}

static int int_iter_next(sam_iter_t *i, sam_word_t *val)
{
    sam_uword_t index = (sam_uword_t)(i->iter.word_state);
    sam_uword_t n = (sam_uword_t)(i->blob);
    if (index == n)
        *val = SAM_VALUE_NULL;
    else {
        *val = SAM_INT_TAG | (index++ << SAM_INT_SHIFT);
        i->iter.word_state = index;
    }
    return SAM_ERROR_OK;
}

int sam_int_iter_new(sam_uword_t n, sam_blob_t **new_iter)
{
    sam_word_t error = SAM_ERROR_OK;
    HALT_IF_ERROR(sam_blob_new(SAM_BLOB_ITER, sizeof(sam_iter_t), new_iter));
    sam_iter_t *i;
    EXTRACT_BLOB(*new_iter, SAM_BLOB_ITER, sam_iter_t, i);
    i->tag = SAM_BLOB_TAG | (SAM_BLOB_MAP << SAM_BLOB_SHIFT);
    i->blob = (sam_blob_t *)n;
    i->next = int_iter_next;
    i->iter.word_state = 0;

error:
    return error;
}