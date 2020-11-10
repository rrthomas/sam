// Private implementation-specific APIs that are shared between modules.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

// Errors
#define HALT(code)                              \
    do {                                        \
        error = code;                           \
        goto error;                             \
    } while (0)

#define HALT_IF_ERROR(code)                     \
    do {                                        \
        if (code != SAM_ERROR_OK)               \
            HALT(code);                         \
    } while (0)


// Stack access
#define POP(ptr)                                                        \
    HALT_IF_ERROR(sam_pop_stack(sam_m0, sam_msize, sam_s0, &sam_sp, ptr))
#define PUSH(v)                                                         \
    HALT_IF_ERROR(sam_push_stack(sam_m0, sam_msize, sam_s0, &sam_sp, v))


// Portable arithmetic right shift (the behaviour of >> on signed
// quantities is implementation-defined in C99)
#if HAVE_ARITHMETIC_RSHIFT
#define ARSHIFT(n, p)                           \
    ((sam_word_t)(n) >> (p))
#else
#define ARSHIFT(n, p)                                                   \
    (((n) >> (p)) | (sam_word_t)(LSHIFT(-((n) < 0), (SAM_WORD_BIT - p))))
#endif

// Traps
sam_word_t trap(sam_word_t code);
sam_word_t trap_tildav2(sam_uword_t function);
