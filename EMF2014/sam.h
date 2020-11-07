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
sam_uword_t sam_stack_item(sam_word_t *m0, sam_uword_t msize, sam_uword_t s0, sam_uword_t sp, sam_uword_t n);
sam_uword_t sam_find_code(sam_uword_t depth);
int sam_pop_stack(sam_word_t *m0, sam_uword_t msize, sam_uword_t s0, sam_uword_t *sp, sam_word_t *val_ptr);
int sam_push_stack(sam_word_t *m0, sam_uword_t msize, sam_uword_t s0, sam_uword_t *sp, sam_word_t val);

// Miscellaneous routines
sam_word_t sam_run(void);
int sam_init(sam_word_t *m0, sam_uword_t msize, sam_uword_t sp);

// Portable left shift (the behaviour of << with overflow (including on any
// negative number) is undefined)
#define LSHIFT(n, p)                            \
    (((n) & (SAM_UWORD_MAX >> (p))) << (p))

// Debug
void sam_print_stack(void);

// Traps
void sam_prim_blit(void);
