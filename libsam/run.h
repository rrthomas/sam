// Handy stack access macros for implementing the interpreter and traps.
//
// (c) Reuben Thomas 1994-2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#define CHECK_TYPE(var, mask, insn)             \
    if ((var & (mask)) != (insn))               \
        HALT(SAM_ERROR_WRONG_TYPE);

#define POP_WORD_UNSAFE(ptr)                    \
    HALT_IF_ERROR(sam_stack_pop_unsafe(s, ptr))
#define POP_WORD(ptr)                           \
    HALT_IF_ERROR(sam_stack_pop(s, ptr))
#define PUSH_WORD(val)                          \
    HALT_IF_ERROR(sam_stack_push(s, val))

#define _POP_INSN(var, pop, insn, insn_mask, rshift, shift)     \
    do {                                        \
        pop((sam_word_t *)&var);                \
        CHECK_TYPE(var, insn_mask, insn);       \
        var = rshift(var, shift);               \
    } while (0)

#define _POP_INT(var, rshift)                   \
    _POP_INSN(var, POP_WORD, SAM_INT_TAG, SAM_INT_TAG_MASK, rshift, SAM_INT_SHIFT)
#define POP_INT(var)                            \
    _POP_INT(var, ARSHIFT)
#define POP_UINT(var)                           \
    _POP_INT(var, LRSHIFT)
#define PUSH_INT(val)                           \
    PUSH_WORD(SAM_INT_TAG | LSHIFT(val, SAM_INT_SHIFT))

#define PUSH_BOOL(val)                          \
    PUSH_INT(-((val) != 0))

#define POP_REF_UNSAFE(var)                                             \
    do {                                                                \
        sam_uword_t _ptr;                                               \
        _POP_INSN(_ptr, POP_WORD_UNSAFE, SAM_STACK_TAG, SAM_STACK_TAG_MASK, LRSHIFT, SAM_STACK_SHIFT); \
        var = (void *)(_ptr << SAM_STACK_SHIFT);                        \
    } while (0)
#define POP_REF(var)                                  \
    do {                                              \
        POP_REF_UNSAFE(var);                          \
        if (var->nrefs <= 1)                          \
            HALT(SAM_ERROR_ORPHAN_STACK);             \
        WIPE_STACK_SLOT(0);                           \
    } while (0)

#define PUSH_REF(addr)                                \
    PUSH_WORD(((sam_uword_t)addr) | SAM_STACK_TAG)

#define POP_FLOAT(var)                                                  \
    do {                                                                \
        sam_uword_t _w;                                                 \
        _POP_INSN(_w, POP_WORD, SAM_FLOAT_TAG, SAM_FLOAT_TAG_MASK, LRSHIFT, SAM_FLOAT_SHIFT); \
        var = *(sam_float_t *)&_w;                                      \
    } while (0)
#define PUSH_FLOAT(val)                         \
    sam_push_float(s, val)
