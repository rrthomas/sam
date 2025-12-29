// SAM's math traps.
//
// (c) Reuben Thomas 2020-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifndef SAM_TRAP_MATH
#define SAM_TRAP_MATH

sam_word_t sam_math_trap(sam_stack_t *s, sam_uword_t function);
char *sam_math_trap_name(sam_word_t function);

#define SAM_TRAP_MATH_BASE 0x100

enum SAM_TRAP_MATH {
    TRAP_MATH_POW = SAM_TRAP_MATH_BASE,
    TRAP_MATH_SIN,
    TRAP_MATH_COS,
    TRAP_MATH_DEG,
    TRAP_MATH_RAD,
};

#endif
