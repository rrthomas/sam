// Sam interpreter APIs and primitives.

#ifndef SAM_SAM
#define SAM_SAM

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

// Basic types
typedef ptrdiff_t sam_word_t;
typedef size_t sam_uword_t;
#if SIZEOF_SIZE_T == 4
#ifdef __STDCPP_FLOAT16_T__
typedef float16_t sam_float_t;
#else
#error No suitable type for sam_float_t
#endif
#else
typedef float sam_float_t;
#endif

#define SAM_WORD_BYTES (sizeof(size_t))
#define SAM_WORD_BIT (SAM_WORD_BYTES * 8)
#define SAM_WORD_MIN ((sam_word_t)(1UL << (SAM_WORD_BIT - 1)))
#define SAM_INT_MIN ((sam_uword_t)SAM_WORD_MIN >> SAM_OPERAND_SHIFT)
#define SAM_UWORD_MAX (SIZE_MAX)

// VM registers
extern sam_uword_t sam_pc;

// Stacks
typedef struct sam_stack {
    sam_word_t *s0;
    sam_uword_t ssize; // Size of stack in words
    sam_uword_t sp; // Number of words (NOT items!) in stack
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
};

// Stack access
sam_stack_t *sam_stack_new(void);
int sam_stack_peek(sam_stack_t *s, sam_uword_t addr, sam_uword_t *val);
int sam_stack_poke(sam_stack_t *s, sam_uword_t addr, sam_uword_t val);
int sam_stack_get(sam_uword_t addr); // FIXME: add stack parameter
int sam_stack_move(sam_uword_t addr); // FIXME: add stack parameter
int sam_stack_item(sam_uword_t s0, sam_uword_t sp, sam_word_t n, sam_uword_t *addr);
int sam_pop_stack(sam_word_t *val_ptr);
int sam_push_stack(sam_stack_t *s, sam_word_t val);
int sam_push_ref(sam_stack_t *s, sam_uword_t addr);
int sam_push_atom(sam_stack_t *s, sam_uword_t atom_type, sam_uword_t operand);
int sam_push_float(sam_stack_t *s, sam_float_t n);
int sam_push_code(sam_stack_t *s, sam_word_t *ptr, sam_uword_t size);

// Miscellaneous routines
sam_word_t sam_run(void);
int sam_init(void);

// Portable left shift (the behaviour of << with overflow (including on any
// negative number) is undefined)
#define LSHIFT(n, p)                            \
    (((n) & (SAM_UWORD_MAX >> (p))) << (p))

// Debug
#ifdef SAM_DEBUG
#include <stdbool.h>
extern bool do_debug;
char *inst_name(sam_word_t inst_opcode);
void sam_print_stack(void);
void sam_print_working_stack(void);
void debug(const char *fmt, ...);
int sam_debug_init(void);
#endif

// Traps
#define SAM_DISPLAY_WIDTH 800
#define SAM_DISPLAY_HEIGHT 600
extern const unsigned sam_update_interval;

sam_word_t sam_traps_init(void);
void sam_traps_finish(void);
int sam_traps_process_events(void);
sam_word_t sam_trap(sam_uword_t function);
int sam_traps_window_used(void);
uint32_t sam_getpixel(int x, int y);
void sam_dump_screen(const char *filename);
void sam_update_screen(void);
void sam_traps_wait(void);

#endif
