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

// Stacks
typedef struct sam_stack {
    sam_word_t *s0;
    sam_uword_t ssize; // Size of stack in words
    sam_uword_t sp; // Number of words in stack
} sam_stack_t;

// Frames
// FIXME: separate out sam_state_t which has a pointer to top frame and
// pointer to current root (i.e. code context).
typedef struct sam_frame {
    struct sam_frame *parent;
    sam_stack_t *code;
    sam_stack_t *stack;
    sam_uword_t pc;
} sam_frame_t;

// Error codes
enum {
    SAM_ERROR_OK = 0,
    SAM_ERROR_HALT = 1,
    SAM_ERROR_INVALID_OPCODE = 2,
    SAM_ERROR_INVALID_ADDRESS = 3,
    SAM_ERROR_STACK_UNDERFLOW = 4,
    SAM_ERROR_STACK_OVERFLOW = 5,
    SAM_ERROR_WRONG_TYPE = 6,
    SAM_ERROR_INVALID_TRAP = 7,
    SAM_ERROR_TRAP_INIT = 8,
    SAM_ERROR_NO_MEMORY = 9,
    SAM_ERROR_INVALID_ARRAY_TYPE = 10,
};

// Stack access
int sam_stack_new(unsigned type, sam_stack_t **new_stack);
int sam_stack_peek(sam_stack_t *s, sam_uword_t addr, sam_uword_t *val);
int sam_stack_poke(sam_stack_t *s, sam_uword_t addr, sam_uword_t val);
int sam_stack_extract(sam_stack_t *s, sam_uword_t addr);
int sam_stack_insert(sam_stack_t *s, sam_uword_t addr);
int sam_stack_item(sam_stack_t *s, sam_word_t n, sam_uword_t *addr);
int sam_pop_stack(sam_stack_t *s, sam_word_t *val_ptr);
int sam_push_stack(sam_stack_t *s, sam_word_t val);
int sam_push_ref(sam_stack_t *s, void *ptr);
int sam_push_int(sam_stack_t *s, sam_uword_t val);
int sam_push_float(sam_stack_t *s, sam_float_t n);
int sam_push_atom(sam_stack_t *s, sam_uword_t atom_type, sam_uword_t operand);
int sam_push_trap(sam_stack_t *s, sam_uword_t function);
int sam_push_insts(sam_stack_t *s, sam_uword_t insts);

// Frames
sam_frame_t *sam_frame_new(void);

// Miscellaneous routines
sam_word_t sam_run(sam_frame_t *f);

// Debug
#ifdef SAM_DEBUG
#include <stdbool.h>
extern bool do_debug;
char *inst_name(sam_uword_t inst_opcode);
char *trap_name(sam_uword_t function);
void sam_print_stack(sam_stack_t *s);
void sam_print_working_stack(sam_stack_t *s);
void debug(const char *fmt, ...);
int sam_debug_init(sam_stack_t *s);
#endif

#endif
