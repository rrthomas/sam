// Activation frames.
//
// (c) Reuben Thomas 2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <stdlib.h>

#include "sam.h"

sam_frame_t *sam_frame_new(void)
{
    return calloc(sizeof(sam_frame_t), 1);
}
