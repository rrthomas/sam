// Sam interpreter APIs and primitives.
//
// (c) Reuben Thomas 1994-2026
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

// Blobs
typedef struct {
    sam_uword_t type;
    _Alignas(max_align_t) sam_word_t data[];
} sam_blob_t;

// Stacks (SAM_BLOB_STACK)
typedef struct {
    sam_word_t *s0;
    sam_uword_t ssize; // Size of stack in words
    sam_uword_t sp; // Number of words in stack
} sam_stack_t;

// Maps (SAM_BLOB_MAP)
typedef struct sam_map_struct *sam_map_t;

// Map iterators
typedef struct sam_map_iter_struct *sam_map_iter_t;

// Top-level state
typedef struct sam_state {
    sam_blob_t *root_code;
    sam_blob_t *stack;
    sam_blob_t *pc0;
    sam_uword_t pc;
} sam_state_t;

// Error codes
enum SAM_ERROR {
    SAM_ERROR_OK,
    SAM_ERROR_INVALID_OPCODE,
    SAM_ERROR_INVALID_ADDRESS,
    SAM_ERROR_STACK_UNDERFLOW,
    SAM_ERROR_STACK_OVERFLOW,
    SAM_ERROR_WRONG_TYPE,
    SAM_ERROR_INVALID_TRAP,
    SAM_ERROR_TRAP_INIT,
    SAM_ERROR_NO_MEMORY,
    SAM_ERROR_INVALID_ATOM_TYPE,
    SAM_ERROR_INVALID_BLOB_TYPE,
};

// Blobs
int sam_blob_new(unsigned type, sam_blob_t **new_blob);

// Stack access
int sam_stack_from_blob(sam_blob_t *blob, sam_stack_t **s);
int sam_stack_new(sam_blob_t **new_stack);
int sam_stack_copy(sam_blob_t *stack, sam_blob_t **new_stack);
int sam_stack_peek(sam_blob_t *s, sam_uword_t addr, sam_uword_t *val);
int sam_stack_poke(sam_blob_t *s, sam_uword_t addr, sam_uword_t val);
int sam_stack_extract(sam_blob_t *s, sam_uword_t addr);
int sam_stack_insert(sam_blob_t *s, sam_uword_t addr);
int sam_stack_item(sam_blob_t *s, sam_word_t n, sam_uword_t *addr);
int sam_stack_pop(sam_blob_t *s, sam_word_t *val_ptr);
int sam_stack_shift(sam_blob_t *s, sam_word_t *val_ptr);
int sam_stack_push(sam_blob_t *s, sam_word_t val);
int sam_stack_prepend(sam_blob_t *s, sam_word_t val);
int sam_push_ref(sam_blob_t *s, void *ptr);
int sam_push_int(sam_blob_t *s, sam_uword_t val);
int sam_push_float(sam_blob_t *s, sam_float_t n);
int sam_push_atom(sam_blob_t *s, sam_uword_t atom_type, sam_uword_t operand);
int sam_push_trap(sam_blob_t *s, sam_uword_t function);
int sam_push_insts(sam_blob_t *s, sam_uword_t insts);

// Maps
int sam_map_new(sam_blob_t **new_map);
int sam_map_copy(sam_blob_t *map, sam_blob_t **new_map);
int sam_map_get(sam_blob_t *blob, sam_word_t key, sam_word_t *val);
int sam_map_set(sam_blob_t *blob, sam_word_t key, sam_word_t val);
int sam_map_iter_new(sam_blob_t *blob, sam_map_iter_t *i);
int sam_map_iter_next(sam_map_iter_t i, sam_word_t *key, sam_word_t *val);

// Top-level states
sam_state_t *sam_state_new(void);

// Miscellaneous routines
sam_word_t sam_run(sam_state_t *state);

// Debug
#ifdef SAM_DEBUG
#include <stdbool.h>
extern bool do_debug;
char *inst_name(sam_uword_t inst_opcode);
char *trap_name(sam_uword_t function);
char *disas(sam_word_t inst);
void sam_print_stack(sam_blob_t *blob);
void sam_print_working_stack(sam_blob_t *blob);
void debug(const char *fmt, ...);
int sam_debug_init(sam_state_t *state);
#endif

#endif
