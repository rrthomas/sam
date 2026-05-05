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
typedef double sam_float_t;

#define SAM_WORD_BYTES (sizeof(size_t))
_Static_assert(sizeof(size_t) == 8);
#define SAM_WORD_BIT (SAM_WORD_BYTES * 8)
#define SAM_WORD_MIN ((sam_word_t)(1UL << (SAM_WORD_BIT - 1)))
#define SAM_INT_MIN ((sam_word_t)((sam_uword_t)SAM_WORD_MIN >> SAM_INT_SHIFT))
#define SAM_UWORD_MAX (SIZE_MAX)
#define SAM_RET_SHIFT 8
#define SAM_RET_MASK ((1 << SAM_RET_SHIFT) - 1)

#define NAME _sam_map
#define KEY_TY uintptr_t
#define VAL_TY uintptr_t
#define HEADER_MODE
#include "verstable.h"
#undef NAME
#undef KEY_TY
#undef VAL_TY
#undef HEADER_MODE

// Blobs
typedef struct sam_blob sam_blob_t;
typedef struct sam_array sam_array_t;
typedef struct sam_closure sam_closure_t;
typedef _sam_map sam_map_t;
typedef struct sam_iter sam_iter_t;
typedef struct sam_string sam_string_t;

// Map iterators
typedef _sam_map_itr sam_map_iter_t;

// Top-level state
typedef struct sam_state sam_state_t;

// Error codes
enum SAM_ERROR {
    SAM_ERROR_OK,
    SAM_ERROR_HALT,
    SAM_ERROR_INVALID_OPCODE,
    SAM_ERROR_INVALID_ADDRESS,
    SAM_ERROR_ARRAY_UNDERFLOW,
    SAM_ERROR_ARRAY_OVERFLOW,
    SAM_ERROR_WRONG_TYPE,
    SAM_ERROR_INVALID_TRAP,
    SAM_ERROR_TRAP_INIT,
    SAM_ERROR_NO_MEMORY,
    SAM_ERROR_INVALID_ATOM_TYPE,
    SAM_ERROR_INVALID_BLOB_TYPE,
};

// Blobs
int sam_blob_new(unsigned type, size_t data_size, sam_blob_t **new_blob);

// Array access
int sam_array_from_blob(sam_blob_t *blob, sam_array_t **s);
int sam_array_new(sam_blob_t **new_array);
int sam_array_copy(sam_blob_t *array, sam_blob_t **new_array);
// FIXME: val in next two functions should be word, not uword
int sam_array_peek(sam_blob_t *s, sam_uword_t addr, sam_uword_t *val);
int sam_array_poke(sam_blob_t *s, sam_uword_t addr, sam_uword_t val);
int sam_array_extract(sam_blob_t *s, sam_uword_t addr);
int sam_array_insert(sam_blob_t *s, sam_uword_t addr);
int sam_array_item(sam_blob_t *s, sam_word_t n, sam_uword_t *addr);
int sam_array_pop(sam_blob_t *s, sam_word_t *val_ptr);
int sam_array_shift(sam_blob_t *s, sam_word_t *val_ptr);
int sam_array_push(sam_blob_t *s, sam_word_t val);
int sam_array_prepend(sam_blob_t *s, sam_word_t val);
int sam_make_inst_blob(sam_word_t *inst, sam_blob_t *val);
int sam_make_inst_int(sam_word_t *inst, sam_word_t val);
int sam_make_inst_float(sam_word_t *inst, sam_float_t n);
int sam_make_inst_atom(sam_word_t *inst, sam_uword_t atom_type, sam_uword_t operand);
int sam_make_inst_trap(sam_word_t *inst, sam_uword_t function);
int sam_make_inst_insts(sam_word_t *inst, sam_uword_t insts);
int sam_array_iter_new(sam_blob_t *blob, sam_blob_t **new_iter);

// Closures
int sam_closure_new(sam_blob_t **new_closure, sam_blob_t *code, sam_blob_t *context);
int sam_closure_iter_new(sam_blob_t *blob, sam_blob_t **new_iter);

// Maps
int sam_map_new(sam_blob_t **new_map);
int sam_map_copy(sam_blob_t *map, sam_blob_t **new_map);
int sam_map_get(sam_blob_t *blob, sam_word_t key, sam_word_t *val);
int sam_map_set(sam_blob_t *blob, sam_word_t key, sam_word_t val);
int sam_map_iter_new(sam_blob_t *blob, sam_blob_t **new_iter);

// Iterators
int sam_iter_next(sam_iter_t *i, sam_word_t *val);
int sam_int_iter_new(sam_uword_t n, sam_blob_t **new_iter);

// Strings
int sam_string_new(sam_blob_t **new_string, const char *cstr, size_t len);
int sam_string_iter_new(sam_blob_t *blob, sam_blob_t **new_iter);

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
void sam_print_array(sam_blob_t *blob);
void sam_print_working_stack(sam_blob_t *blob);
void debug(const char *fmt, ...);
#endif

#endif
