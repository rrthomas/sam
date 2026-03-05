// SAM maps.
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


#define NAME _sam_map
#define KEY_TY uintptr_t
#define VAL_TY uintptr_t
#define IMPLEMENTATION_MODE
#include "verstable.h"

typedef _sam_map sam_map_t;

typedef _sam_map_itr sam_map_iter_t;


int sam_map_new(sam_blob_t **new_map)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_blob_t *blob;
    HALT_IF_ERROR(sam_blob_new(SAM_BLOB_MAP, sizeof(_sam_map), &blob));
    sam_map_t *m;
    EXTRACT_BLOB(blob, SAM_BLOB_MAP, sam_map_t, m);
    vt_init(m);
    *new_map = blob;

error:
    return error;
}

int sam_map_copy(sam_blob_t *map, sam_blob_t **new_map)
{
    int error = SAM_ERROR_OK;
    HALT_IF_ERROR(sam_map_new(new_map));

    // Copy the contents of the map.
    sam_map_iter_t *i;
    HALT_IF_ERROR(sam_map_iter_new(map, &i));
    for (;;) {
        sam_word_t key, val;
        HALT_IF_ERROR(sam_map_iter_next(i, &key, &val));
        if (val == ((SAM_ATOM_NULL << SAM_ATOM_TYPE_SHIFT) | SAM_ATOM_TAG))
            break;
        HALT_IF_ERROR(sam_map_set(*new_map, key, val));
    }

 error:
    return error;
}

int sam_map_get(sam_blob_t *blob, sam_word_t key, sam_word_t *val)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_map_t *m;
    EXTRACT_BLOB(blob, SAM_BLOB_MAP, sam_map_t, m);
    _sam_map_itr itr = vt_get(m, key);
    if (vt_is_end(itr))
       *val = (SAM_ATOM_NULL << SAM_ATOM_TYPE_SHIFT) | SAM_ATOM_TAG;
    *val = itr.data->val;

error:
    return error;
}

int sam_map_set(sam_blob_t *blob, sam_word_t key, sam_word_t val)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_map_t *m;
    EXTRACT_BLOB(blob, SAM_BLOB_MAP, sam_map_t, m);
    if (val == ((SAM_ATOM_NULL << SAM_ATOM_TYPE_SHIFT) | SAM_ATOM_TAG)) {
        vt_erase(m, key);
    } else {
        _sam_map_itr itr = vt_insert(m, key, val);
        if (vt_is_end(itr))
            HALT(SAM_ERROR_NO_MEMORY);
    }

error:
    return error;
}

int sam_map_iter_new(sam_blob_t *blob, sam_map_iter_t **i)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_map_t *m;
    EXTRACT_BLOB(blob, SAM_BLOB_MAP, sam_map_t, m);
    *i = malloc(sizeof(_sam_map_itr));
    // FIXME: check for NULL
    **i = vt_first(m);

error:
    return error;
}

int sam_map_iter_next(sam_map_iter_t *i, sam_word_t *key, sam_word_t *val)
{
    if (vt_is_end(*i)) {
        *key = *val = (SAM_ATOM_NULL << SAM_ATOM_TYPE_SHIFT) | SAM_ATOM_TAG;
    } else {
        *key = i->data->key;
        *val = i->data->val;
        *i = vt_next(*i);
    }
    return SAM_ERROR_OK;
}
