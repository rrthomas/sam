// Private implementation-specific APIs that are shared between modules.
//
// (c) Reuben Thomas 1994-2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include "sam.h"

// Structs
typedef struct sam_blob {
    sam_uword_t type;
    _Alignas(max_align_t) sam_word_t data[];
} sam_blob_t;

typedef struct sam_stack {
    sam_word_t *data;
    sam_uword_t size; // Size of stack in words
    sam_uword_t sp; // Number of words in stack
} sam_stack_t;

typedef struct sam_state {
    sam_blob_t *s0;
    sam_blob_t *pc0;
    sam_uword_t pc;
} sam_state_t;

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

#define CHECK_BLOB(blob, type_code)                     \
    do {                                                \
        if ((blob)->type != type_code)                  \
            HALT(SAM_ERROR_WRONG_TYPE);                 \
    } while (0)

#define EXTRACT_BLOB(blob, type_code, blob_type, var)   \
    do {                                                \
        CHECK_BLOB((blob), type_code);                  \
        var = (blob_type *)(&((blob)->data));           \
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
