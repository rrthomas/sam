// Sam interpreter APIs and primitives.

#include <stdint.h>
#include <limits.h>

// Basic types
typedef int32_t sam_word_t;
typedef uint32_t sam_uword_t;
typedef float sam_float_t;
#define SAM_WORD_BYTES 4
#define SAM_WORD_BIT (SAM_WORD_BYTES * 8)
#define SAM_WORD_MIN ((sam_word_t)(1UL << (SAM_WORD_BIT - 1)))
#define SAM_UWORD_MAX (UINT32_MAX)

// VM registers
#define R(reg, type)                            \
    extern type sam_##reg;
#include "sam_registers.h"
#undef R

// Error codes
enum {
    SAM_ERROR_OK = 0,
    SAM_ERROR_INVALID_OPCODE = -1,
    SAM_ERROR_STACK_UNDERFLOW = -2,
    SAM_ERROR_STACK_OVERFLOW = -3,
    SAM_ERROR_INVALID_LOAD = -4,
    SAM_ERROR_INVALID_STORE = -5,
    SAM_ERROR_UNALIGNED_ADDRESS = -6,
    SAM_ERROR_INVALID_LIBRARY = -16,
    SAM_ERROR_INVALID_FUNCTION = -17,
    SAM_ERROR_BREAK = -256,
};

// Stack access
int sam_pop_stack(sam_word_t *s0, sam_uword_t ssize, sam_uword_t *sp, sam_word_t *val_ptr);
int sam_push_stack(sam_word_t *s0, sam_uword_t ssize, sam_uword_t *sp, sam_word_t val);

// Miscellaneous routines
sam_word_t sam_run(void);
int sam_init(sam_word_t *c_array, sam_uword_t stack_size);

void sam_prim_blit(void);
