// Sam interpreter APIs and primitives.

#ifndef SAM_SAM
#define SAM_SAM

#include <stdint.h>
#include <limits.h>

// Basic types
typedef int32_t sam_word_t;
typedef uint32_t sam_uword_t;
typedef float sam_float_t;
#define SAM_WORD_BYTES 4
#define SAM_WORD_BIT (SAM_WORD_BYTES * 8)
#define SAM_WORD_MIN ((sam_word_t)(1UL << (SAM_WORD_BIT - 1)))
#define SAM_INT_MIN ((sam_uword_t)SAM_WORD_MIN >> SAM_OPERAND_SHIFT)
#define SAM_UWORD_MAX (UINT32_MAX)

// VM registers
#define R(reg, type)                            \
    extern type sam_##reg;
#include "sam_registers.h"
#undef R

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
    SAM_ERROR_UNPAIRED_BIATOM = 8,
    SAM_ERROR_INVALID_FUNCTION = 9,
    SAM_ERROR_TRAP_INIT = 10,
};

// Stack access
int sam_stack_peek(sam_uword_t addr, sam_uword_t *val);
int sam_stack_poke(sam_uword_t addr, sam_uword_t val);
int sam_stack_push(sam_uword_t addr, sam_uword_t size);
int sam_stack_set(sam_uword_t addr1, sam_uword_t size1, sam_uword_t addr2, sam_uword_t size2);
int sam_stack_item(sam_uword_t s0, sam_uword_t sp, sam_word_t n, sam_uword_t *addr, sam_uword_t *size);
int sam_pop_stack(sam_word_t *val_ptr);
int sam_push_stack(sam_word_t val);

// Miscellaneous routines
sam_word_t sam_run(void);
int sam_init(sam_word_t *m0, sam_uword_t msize, sam_uword_t sp);

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
