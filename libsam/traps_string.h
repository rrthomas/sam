// Sam string traps.
//
// (c) Reuben Thomas 2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifndef SAM_TRAP_STRING
#define SAM_TRAP_STRING

#include "sam.h"

sam_word_t sam_string_trap(sam_state_t *state, sam_uword_t function);
char *sam_string_trap_name(sam_word_t function);

#define SAM_TRAP_STRING_BASE 0x300

enum SAM_TRAP_STRING {
  TRAP_STRING_NEW_STRING = SAM_TRAP_STRING_BASE,
  TRAP_STRING_PRINT,
};

#endif
