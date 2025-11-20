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
#define CHECK_BIATOM_FIRST(first, type)         \
    do {                                        \
        if ((first & (SAM_TAG_MASK | SAM_BIATOM_TAG_MASK)) != (SAM_TAG_BIATOM | (SAM_BIATOM_FIRST << SAM_BIATOM_TAG_SHIFT)) || \
            ((first & SAM_BIATOM_TYPE_MASK) >> SAM_BIATOM_TYPE_SHIFT) != type) \
            HALT(SAM_ERROR_UNPAIRED_BIATOM);    \
    } while(0)
#define CHECK_BIATOM_SECOND(second, type)       \
    do {                                        \
        if ((second & (SAM_TAG_MASK | SAM_BIATOM_TAG_MASK)) != (SAM_TAG_BIATOM | (SAM_BIATOM_SECOND << SAM_BIATOM_TAG_SHIFT)) || \
            ((second & SAM_BIATOM_TYPE_MASK) >> SAM_BIATOM_TYPE_SHIFT) != type) \
            HALT(SAM_ERROR_UNPAIRED_BIATOM);    \
    } while(0)
#define POP_BIATOM(first, second_var)           \
    do {                                        \
        POP_WORD(&second_var);                  \
        if ((first & SAM_TAG_BIATOM) != SAM_BIATOM_FIRST || \
            (first & SAM_BIATOM_TYPE_MASK) != (second_var & SAM_BIATOM_TYPE_MASK)) \
            HALT(SAM_ERROR_UNPAIRED_BIATOM);    \
        CHECK_BIATOM_SECOND(second_var, (second_var & SAM_BIATOM_TYPE_MASK) >> SAM_BIATOM_TYPE_SHIFT); \
    } while(0)

#define _POP_INSN(var, insn, insn_mask, op_mask, rshift, shift)     \
    do {                                        \
        POP_WORD((sam_word_t *)&var);           \
        _CHECK_TYPE(var, insn_mask, insn); \
        var = rshift(var & op_mask, shift); \
    } while (0)
#define _PUSH_INSN(val, insn)                   \
    PUSH_WORD(LSHIFT((val), SAM_OPERAND_SHIFT) | insn)

#define _POP_INT(var, rshift)                   \
    _POP_INSN(var, SAM_TAG_ATOM | (SAM_ATOM_INT << SAM_ATOM_TYPE_SHIFT), SAM_TAG_MASK | SAM_ATOM_TYPE_MASK, SAM_OPERAND_MASK, rshift, SAM_OPERAND_SHIFT)
#define POP_INT(var)                            \
    _POP_INT(var, ARSHIFT)
#define POP_UINT(var)                           \
    _POP_INT(var, LRSHIFT)
#define PUSH_INT(val)                           \
    _PUSH_INSN(val, SAM_TAG_ATOM | (SAM_ATOM_INT << SAM_ATOM_TYPE_SHIFT))

int _sam_get_stack(sam_uword_t *addr);

#define POP_PTR(var)                                  \
    _POP_INSN(var, SAM_TAG_LINK, SAM_TAG_MASK, SAM_LINK_MASK, LRSHIFT, SAM_LINK_SHIFT)
#define POP_LINK(var)                                 \
    do {                                              \
        POP_PTR(var);                                 \
        sam_uword_t _stack;                           \
        HALT_IF_ERROR(sam_stack_peek(sam_stack, var, &_stack));  \
        _CHECK_TYPE(_stack, SAM_TAG_MASK | SAM_ARRAY_TYPE_MASK, SAM_TAG_ARRAY | SAM_ARRAY_STACK); \
        var++;                                        \
    } while(0)
#define PUSH_PTR(addr)                                     \
    PUSH_WORD(LSHIFT((addr), SAM_LINK_SHIFT) | SAM_TAG_LINK)

#define POP_STACK(addr_var, size_var)                     \
    do {                                                  \
        POP_LINK(addr_var);                               \
        sam_uword_t _insn;                                \
        HALT_IF_ERROR(sam_stack_peek(sam_stack, addr_var - 1, &_insn)); \
        size_var = (ARSHIFT(_insn, SAM_OPERAND_SHIFT)) - 1; \
    } while(0)

#define POP_FLOAT(var)                                                  \
    do {                                                                \
        sam_uword_t w2;                                                 \
        POP_WORD((sam_word_t *)&w2);                                    \
        _CHECK_TYPE(w2, SAM_TAG_MASK | SAM_BIATOM_TAG_MASK | SAM_BIATOM_TYPE_MASK, SAM_TAG_BIATOM | (SAM_BIATOM_SECOND << SAM_BIATOM_TAG_SHIFT) | (SAM_BIATOM_FLOAT << SAM_BIATOM_TYPE_SHIFT)); \
        sam_uword_t w1;                                                 \
        POP_WORD((sam_word_t *)&w1);                                    \
        _CHECK_TYPE(w1, SAM_TAG_MASK | SAM_BIATOM_TAG_MASK | SAM_BIATOM_TYPE_MASK, SAM_TAG_BIATOM | (SAM_BIATOM_FIRST << SAM_BIATOM_TAG_SHIFT) | (SAM_BIATOM_FLOAT << SAM_BIATOM_TYPE_SHIFT)); \
        w1 = ((w1 & SAM_OPERAND_MASK) | ((w2 >> SAM_OPERAND_SHIFT) & ~SAM_OPERAND_MASK)); \
        var = *(sam_float_t *)&w1;                                      \
    } while (0)

#define PUSH_FLOAT(val)                                                 \
    do {                                                                \
        sam_float_t v = val;                                            \
        _PUSH_INSN((*(sam_uword_t *)&v >> SAM_OPERAND_SHIFT), SAM_TAG_BIATOM | (SAM_BIATOM_FIRST << SAM_BIATOM_TAG_SHIFT) | (SAM_BIATOM_FLOAT << SAM_BIATOM_TYPE_SHIFT)); \
        _PUSH_INSN((*(sam_uword_t *)&v & ~SAM_OPERAND_MASK), SAM_TAG_BIATOM | (SAM_BIATOM_SECOND << SAM_BIATOM_TAG_SHIFT) | (SAM_BIATOM_FLOAT << SAM_BIATOM_TYPE_SHIFT)); \
    } while (0)

// Execution
#define DO(addr)                                \
    do {                                        \
        PUSH_PTR(sam_pc0);                      \
        PUSH_PTR(sam_pc);                       \
        sam_pc0 = sam_pc = addr;                \
    } while (0)
#define RET                                                             \
    do {                                                                \
        POP_PTR(sam_pc);                                                \
        POP_PTR(sam_pc0);                                               \
        if (sam_pc0 == 0)                                               \
            HALT(SAM_ERROR_STACK_UNDERFLOW);                            \
        if (sam_pc0 > sam_stack->sp)                                    \
            HALT(SAM_ERROR_STACK_OVERFLOW);                             \
        sam_uword_t _stack, _size;                                      \
        HALT_IF_ERROR(sam_stack_peek(sam_stack, sam_pc0 - 1, &_stack)); \
        _CHECK_TYPE(_stack, SAM_TAG_MASK | SAM_ARRAY_TYPE_MASK, SAM_TAG_ARRAY | SAM_ARRAY_STACK); \
        _size = (ARSHIFT(_stack, SAM_OPERAND_SHIFT)) - 1;               \
        if (sam_pc > sam_pc0 + _size)                                   \
            HALT(SAM_ERROR_STACK_OVERFLOW);                             \
    } while (0)

// Program
#define SAM_STACK_WORDS 4096
extern sam_word_t SAM_PROGRAM[SAM_STACK_WORDS];
extern sam_uword_t sam_program_len;
