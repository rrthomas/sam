// Top-level state.
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

sam_state_t *sam_state_new(void)
{
    return calloc(sizeof(sam_state_t), 1);
}
