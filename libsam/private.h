// Private implementation-specific APIs that are shared between modules.
//
// (c) Reuben Thomas 1994-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include "sam.h"

// Errors
#define HALT(code)                              \
    do {                                        \
        error = code;                           \
        goto error;                             \
    } while (0)

#define HALT_IF_ERROR(code)                     \
    do {                                        \
        sam_word_t _err = code;                 \
        if (_err != SAM_ERROR_OK)               \
            HALT(_err);                         \
    } while (0)

// Portable left shift (the behaviour of << with overflow (including on any
// negative number) is undefined)
#define LSHIFT(n, p)                            \
    (((n) & (SAM_UWORD_MAX >> (p))) << (p))

// Portable arithmetic right shift (the behaviour of >> on signed
// quantities is implementation-defined in C99)
#define ARSHIFT(n, p)                                                   \
    (((n) >> (p)) | (sam_word_t)(LSHIFT(-((n) < 0), (SAM_WORD_BIT - p))))

// Matching macro for logical shift
#define LRSHIFT(x, p)                           \
    ((x) >> (p))
