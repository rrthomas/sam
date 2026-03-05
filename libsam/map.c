// SAM maps.
//
// (c) Reuben Thomas 2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#define NAME _sam_map
#define KEY_TY uintptr_t
#define VAL_TY uintptr_t
#include "verstable.h"

typedef struct sam_map_struct {
    _sam_map map;
} *sam_map_t;

typedef struct sam_map_iter_struct {
    _sam_map_itr itr;
} *sam_map_iter_t;

#include "sam.h"
#include "sam_opcodes.h"

#include "private.h"


int sam_map_new(sam_blob_t **new_map)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_blob_t *blob;
    HALT_IF_ERROR(sam_blob_new(SAM_BLOB_MAP, &blob));
    sam_map_t *m;
    EXTRACT_BLOB(blob, SAM_BLOB_MAP, sam_map_t, m);
    *m = malloc(sizeof(struct sam_map_struct));
    // FIXME: check for NULL
    vt_init(&(*m)->map);
    *new_map = blob;

error:
    return error;
}

int sam_map_copy(sam_blob_t *map, sam_blob_t **new_map)
{
    int error = SAM_ERROR_OK;
    HALT_IF_ERROR(sam_map_new(new_map));

    // Copy the contents of the map.
    sam_map_iter_t i;
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
    _sam_map_itr itr = vt_get(&(*m)->map, key);
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
        vt_erase(&(*m)->map, key);
    } else {
        _sam_map_itr itr = vt_insert(&(*m)->map, key, val);
        if (vt_is_end(itr))
            HALT(SAM_ERROR_NO_MEMORY);
    }

error:
    return error;
}

int sam_map_iter_new(sam_blob_t *blob, sam_map_iter_t *i)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_map_t *m;
    EXTRACT_BLOB(blob, SAM_BLOB_MAP, sam_map_t, m);
    *i = malloc(sizeof(struct sam_map_iter_struct));
    // FIXME: check for NULL
    (*i)->itr = vt_first(&(*m)->map);

error:
    return error;
}

int sam_map_iter_next(sam_map_iter_t i, sam_word_t *key, sam_word_t *val)
{
    if (vt_is_end(i->itr)) {
        *key = *val = (SAM_ATOM_NULL << SAM_ATOM_TYPE_SHIFT) | SAM_ATOM_TAG;
    } else {
        *key = i->itr.data->key;
        *val = i->itr.data->val;
        i->itr = vt_next(i->itr);
    }
    return SAM_ERROR_OK;
}
