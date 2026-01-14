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
    sam_state_t *state = calloc(sizeof(sam_state_t), 1);
    if (state == NULL)
        return NULL;
    state->root_frame = sam_frame_new();
    if (state->root_frame == NULL) {
        free(state);
        return NULL;
    }
    return state;
}
