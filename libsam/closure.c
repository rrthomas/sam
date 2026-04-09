// SAM closures.
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


int sam_closure_new(sam_blob_t **new_closure, sam_blob_t *code, sam_blob_t *context)
{
    sam_word_t error = SAM_ERROR_OK;
    sam_blob_t *blob;
    HALT_IF_ERROR(sam_blob_new(SAM_BLOB_CLOSURE, sizeof(sam_closure_t), &blob));
    sam_closure_t *cl;
    EXTRACT_BLOB(blob, SAM_BLOB_CLOSURE, sam_closure_t, cl);
    cl->code = code;
    cl->context = context;
    *new_closure = blob;

error:
    return error;
}
