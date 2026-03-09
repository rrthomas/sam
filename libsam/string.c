// SAM strings.
//
// (c) Reuben Thomas 2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <grapheme.h>

#include "sam.h"
#include "sam_opcodes.h"

#include "private.h"


int sam_string_new(sam_blob_t **new_string, const char *cstr, size_t len)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_blob_t *blob;
    HALT_IF_ERROR(sam_blob_new(SAM_BLOB_STRING, sizeof(sam_string_t), &blob));
    sam_string_t *str;
    EXTRACT_BLOB(blob, SAM_BLOB_STRING, sam_string_t, str);
    str->str = strndup(cstr, len);
    if (str->str == NULL) {
        free(blob);
        HALT(SAM_ERROR_NO_MEMORY);
    }
    str->len = len;
    *new_string = blob;

error:
    return error;
}

static int iter_next(sam_iter_t *i, sam_word_t *val)
{
    sam_word_t error = SAM_ERROR_OK;
    char *pos = (char *)i->iter.ptr_state;
    sam_string_t *str;
    EXTRACT_BLOB(i->blob, SAM_BLOB_STRING, sam_string_t, str);
    size_t len_remaining = str->len - (pos - str->str);
    size_t grapheme_len = grapheme_next_character_break_utf8(pos, len_remaining);
    if (grapheme_len == len_remaining) {
        *val = SAM_VALUE_NULL;
    } else {
        sam_blob_t *blob;
        HALT_IF_ERROR(sam_string_new(&blob, pos, grapheme_len));
        *val = SAM_BLOB_TAG | (sam_uword_t)blob;
        i->iter.ptr_state += grapheme_len;
    }

error:
    return error;
}

int sam_string_iter_new(sam_blob_t *blob, sam_blob_t **new_iter)
{
    sam_word_t error = SAM_ERROR_OK;

    sam_string_t *str;
    EXTRACT_BLOB(blob, SAM_BLOB_STRING, sam_string_t, str);

    HALT_IF_ERROR(sam_blob_new(SAM_BLOB_ITER, sizeof(sam_iter_t), new_iter));
    sam_iter_t *i;
    EXTRACT_BLOB(*new_iter, SAM_BLOB_ITER, sam_iter_t, i);
    i->blob = blob;
    i->next = iter_next;
    i->iter.ptr_state = str->str;

error:
    return error;
}
