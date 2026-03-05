// SAM blobs.
//
// (c) Reuben Thomas 2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <stdlib.h>

#include "sam.h"
#include "sam_opcodes.h"

#include "private.h"


int sam_blob_new(unsigned type, sam_blob_t **new_blob)
{
    sam_word_t error = SAM_ERROR_OK;
    if (type > SAM_BLOB_TYPES)
        HALT(SAM_ERROR_INVALID_BLOB_TYPE);
    sam_blob_t *blob = calloc(1, sizeof(sam_blob_t) + sizeof(sam_stack_t));
    if (blob == NULL)
        HALT(SAM_ERROR_NO_MEMORY);
    blob->type = type;
    *new_blob = blob;
error:
    return error;
}
