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

// Portable arithmetic right shift (the behaviour of >> on signed
// quantities is implementation-defined in C99)
#define ARSHIFT(n, p)                                                   \
    (((n) >> (p)) | (sam_word_t)(LSHIFT(-((n) < 0), (SAM_WORD_BIT - p))))

// Matching macro for logical shift
#define LRSHIFT(x, p)                           \
    ((x) >> (p))

// Stack access
#define _CHECK_TYPE(var, mask, insn)            \
    if ((var & (mask)) != (insn))               \
        HALT(SAM_ERROR_WRONG_TYPE);

#define POP_WORD(ptr)                           \
    HALT_IF_ERROR(sam_pop_stack(ptr))
#define PUSH_WORD(val)                          \
    HALT_IF_ERROR(sam_push_stack(sam_stack, val))

#define _POP_INSN(var, insn, insn_mask, rshift, shift) \
    do {                                        \
        POP_WORD((sam_word_t *)&var);           \
        _CHECK_TYPE(var, insn_mask, insn);      \
        var = rshift(var, shift);               \
    } while (0)

#define _POP_INT(var, rshift)                   \
    _POP_INSN(var, SAM_INT_TAG, SAM_INT_TAG_MASK, rshift, SAM_INT_SHIFT)
#define POP_INT(var)                            \
    _POP_INT(var, ARSHIFT)
#define POP_UINT(var)                           \
    _POP_INT(var, LRSHIFT)
#define PUSH_INT(val)                           \
    PUSH_WORD(SAM_INT_TAG | LSHIFT(val, SAM_INT_SHIFT))

#define PUSH_BOOL(val)                          \
    PUSH_INT(-((val) != 0))

#define POP_PTR(var)                                  \
    _POP_INSN(var, SAM_REF_TAG, SAM_REF_TAG_MASK, LRSHIFT, SAM_REF_SHIFT)
#define POP_REF(var)                                  \
    do {                                              \
        POP_PTR(var);                                 \
        sam_uword_t _stack;                           \
        HALT_IF_ERROR(sam_stack_peek(sam_stack, var, &_stack));  \
        _CHECK_TYPE(_stack, SAM_ARRAY_TAG_MASK | SAM_ARRAY_TYPE_MASK, SAM_ARRAY_TAG | (SAM_ARRAY_STACK << SAM_ARRAY_TYPE_SHIFT)); \
        var++;                                        \
    } while(0)
#define PUSH_PTR(addr)                                \
    PUSH_WORD(LSHIFT((addr), SAM_REF_SHIFT) | SAM_REF_TAG)

#define POP_STACK(addr_var, size_var)                     \
    do {                                                  \
        POP_REF(addr_var);                                \
        sam_uword_t _insn;                                \
        HALT_IF_ERROR(sam_stack_peek(sam_stack, addr_var - 1, &_insn)); \
        size_var = (_insn >> SAM_ARRAY_OFFSET_SHIFT) - 1; \
    } while(0)

#define POP_FLOAT(var)                                                  \
    do {                                                                \
        sam_uword_t _w;                                                 \
        _POP_INSN(_w, SAM_FLOAT_TAG, SAM_FLOAT_TAG_MASK, LRSHIFT, SAM_FLOAT_SHIFT); \
        var = *(sam_float_t *)&_w;                                      \
    } while (0)
#define PUSH_FLOAT(val)                         \
    sam_push_float(sam_stack, val)

// Execution
#define DO(addr)                                \
    do {                                        \
        PUSH_PTR(sam_pc);                       \
        sam_pc = addr;                          \
    } while (0)
#define RET                                     \
    POP_PTR(sam_pc)

// Program
#define SAM_STACK_WORDS 4096
extern sam_word_t SAM_PROGRAM[SAM_STACK_WORDS];
extern sam_uword_t sam_program_len;
