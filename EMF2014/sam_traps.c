// The traps.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include "sam.h"

#include "sam_private.h"
#include "sam_traps.h"


sam_word_t trap_tildav2(sam_uword_t function)
{
    int error = SAM_ERROR_OK;
    switch (function) {
    case TRAP_TILDAV2_BLIT: // ( a-addr -- u )
        {
        }
        break;
    default:
        error = SAM_ERROR_INVALID_FUNCTION;
        break;
    }

 /* error: */
    return error;
}

sam_word_t trap(sam_word_t code)
{
    int error = SAM_ERROR_OK;
    switch (code) {
    case TRAP_TILDAV2:
        {
            sam_uword_t function = 0;
            POP((sam_word_t *)&function);
            return trap_tildav2(function);
        }
        break;
    default:
        return SAM_ERROR_INVALID_LIBRARY;
        break;
    }

 error:
    return error;
}
