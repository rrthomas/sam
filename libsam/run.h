// Handy stack access macros for implementing the interpreter and traps.
//
// (c) Reuben Thomas 1994-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#define _CHECK_TYPE(var, mask, insn)            \
    if ((var & (mask)) != (insn))               \
        HALT(SAM_ERROR_WRONG_TYPE);

#define POP_WORD(ptr)                           \
    HALT_IF_ERROR(sam_pop_stack(f->stack, ptr))
#define PUSH_WORD(val)                          \
    HALT_IF_ERROR(sam_push_stack(f->stack, val))

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
        HALT_IF_ERROR(sam_stack_peek(f->stack, var, &_stack)); \
        _CHECK_TYPE(_stack, SAM_ARRAY_TAG_MASK | SAM_ARRAY_TYPE_MASK, SAM_ARRAY_TAG | (SAM_ARRAY_STACK << SAM_ARRAY_TYPE_SHIFT)); \
        var++;                                        \
    } while(0)
#define PUSH_PTR(addr)                                \
    PUSH_WORD(LSHIFT((addr), SAM_REF_SHIFT) | SAM_REF_TAG)

#define POP_FLOAT(var)                                                  \
    do {                                                                \
        sam_uword_t _w;                                                 \
        _POP_INSN(_w, SAM_FLOAT_TAG, SAM_FLOAT_TAG_MASK, LRSHIFT, SAM_FLOAT_SHIFT); \
        var = *(sam_float_t *)&_w;                                      \
    } while (0)
#define PUSH_FLOAT(val)                         \
    sam_push_float(f->stack, val)
