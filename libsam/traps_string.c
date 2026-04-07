// SAM's string traps.
//
// (c) Reuben Thomas 2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <stdbool.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "private.h"
#include "run.h"
#include "traps_string.h"


sam_word_t sam_string_trap(sam_state_t *state, sam_uword_t function)
{
#define s ((sam_array_t *)state->s0->data)
    int error = SAM_ERROR_OK;

    switch (function) {
    case TRAP_STRING_NEW_STRING:
        {
            sam_blob_t *str;
            HALT_IF_ERROR(sam_string_new(&str, "", 0));
            HALT_IF_ERROR(sam_push_blob(state->s0, str));
        }
        break;
    default:
        error = SAM_ERROR_INVALID_TRAP;
        break;
    }

 error:
    return error;
}

char *sam_string_trap_name(sam_word_t function)
{
    switch (function) {
    case TRAP_STRING_NEW_STRING:
        return "NEW_STRING";
    default:
        return NULL;
    }
}
