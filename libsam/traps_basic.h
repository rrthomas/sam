// SAM's basic traps.
//
// (c) Reuben Thomas 2020-2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifndef SAM_TRAP_BASIC
#define SAM_TRAP_BASIC

#include "sam.h"

sam_word_t sam_basic_trap(sam_state_t *state, sam_uword_t function);
char *sam_basic_trap_name(sam_word_t function);

#define SAM_TRAP_BASIC_BASE 0x0

enum SAM_TRAP_BASIC {
    TRAP_BASIC_S0 = SAM_TRAP_BASIC_BASE,
    TRAP_BASIC_QUOTE,
    TRAP_BASIC_NEW,
    TRAP_BASIC_COPY,
    TRAP_BASIC_RET,
    TRAP_BASIC_LSH,
    TRAP_BASIC_RSH,
    TRAP_BASIC_ARSH,
};

#endif
