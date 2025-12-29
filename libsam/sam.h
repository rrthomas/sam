// Sam interpreter APIs and primitives.
//
// (c) Reuben Thomas 1994-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifndef SAM_SAM
#define SAM_SAM

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

// Basic types
typedef ptrdiff_t sam_word_t;
typedef size_t sam_uword_t;
#if SIZE_MAX == 4294967295ULL
typedef float sam_float_t;
#elif SIZE_MAX == 18446744073709551615ULL
typedef double sam_float_t;
#else
#error "SAM needs 4- or 8-byte size_t"
#endif

#define SAM_WORD_BYTES (sizeof(size_t))
#define SAM_WORD_BIT (SAM_WORD_BYTES * 8)
#define SAM_WORD_MIN ((sam_word_t)(1UL << (SAM_WORD_BIT - 1)))
#define SAM_INT_MIN ((sam_uword_t)SAM_WORD_MIN >> SAM_INT_SHIFT)
#define SAM_UWORD_MAX (SIZE_MAX)
#define SAM_RET_SHIFT 8
#define SAM_RET_MASK ((1 << SAM_RET_SHIFT) - 1)

// VM registers
extern sam_uword_t sam_pc;

// Stacks
typedef struct sam_stack {
    sam_word_t *s0;
    sam_uword_t ssize; // Size of stack in words
    sam_uword_t sp; // Number of words in stack
} sam_stack_t;
extern sam_stack_t *sam_stack;

// Error codes
enum {
    SAM_ERROR_OK = 0,
    SAM_ERROR_HALT = 1,
    SAM_ERROR_INVALID_OPCODE = 2,
    SAM_ERROR_INVALID_ADDRESS = 3,
    SAM_ERROR_STACK_UNDERFLOW = 4,
    SAM_ERROR_STACK_OVERFLOW = 5,
    SAM_ERROR_WRONG_TYPE = 6,
    SAM_ERROR_BAD_BRACKET = 7,
    SAM_ERROR_INVALID_TRAP = 8,
    SAM_ERROR_TRAP_INIT = 9,
    SAM_ERROR_NO_MEMORY = 10,
    SAM_ERROR_MOVE_ARRAY = 11,
    SAM_ERROR_INVALID_ARRAY_TYPE = 12,
};

// Stack access
sam_stack_t *sam_stack_new(void);
int sam_stack_peek(sam_stack_t *s, sam_uword_t addr, sam_uword_t *val);
int sam_stack_poke(sam_stack_t *s, sam_uword_t addr, sam_uword_t val);
int sam_stack_get(sam_uword_t addr); // FIXME: add stack parameter
int sam_stack_extract(sam_uword_t addr); // FIXME: add stack parameter
int sam_stack_insert(sam_uword_t addr); // FIXME: add stack parameter
int sam_stack_item(sam_uword_t s0, sam_uword_t sp, sam_word_t n, sam_uword_t *addr);
int sam_pop_stack(sam_word_t *val_ptr);
int sam_push_stack(sam_stack_t *s, sam_word_t val);
int sam_push_ref(sam_stack_t *s, sam_uword_t addr);
int sam_push_int(sam_stack_t *s, sam_uword_t val);
int sam_push_float(sam_stack_t *s, sam_float_t n);
int sam_push_atom(sam_stack_t *s, sam_uword_t atom_type, sam_uword_t operand);
int sam_push_trap(sam_stack_t *s, sam_uword_t function);
int sam_push_code(sam_stack_t *s, sam_word_t *ptr, sam_uword_t size);
int sam_push_insts(sam_stack_t *s, sam_uword_t insts);

// Miscellaneous routines
sam_word_t sam_run(void);
int sam_init(void);
sam_word_t sam_trap(sam_uword_t function);

// Portable left shift (the behaviour of << with overflow (including on any
// negative number) is undefined)
#define LSHIFT(n, p)                            \
    (((n) & (SAM_UWORD_MAX >> (p))) << (p))

// Debug
#ifdef SAM_DEBUG
#include <stdbool.h>
extern bool do_debug;
char *inst_name(sam_uword_t inst_opcode);
char *trap_name(sam_uword_t function);
void sam_print_stack(void);
void sam_print_working_stack(void);
void debug(const char *fmt, ...);
int sam_debug_init(void);
#endif

#endif
