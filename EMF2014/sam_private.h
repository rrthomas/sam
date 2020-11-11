// Private implementation-specific APIs that are shared between modules.
//
// (c) Reuben Thomas 1994-2020
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

// Errors
#define HALT(code)                              \
    do {                                        \
        error = code;                           \
        goto error;                             \
    } while (0)

#define HALT_IF_ERROR(code)                     \
    do {                                        \
        if (code != SAM_ERROR_OK)               \
            HALT(code);                         \
    } while (0)


// Portable arithmetic right shift (the behaviour of >> on signed
// quantities is implementation-defined in C99)
#if HAVE_ARITHMETIC_RSHIFT
#define ARSHIFT(n, p)                           \
    ((sam_word_t)(n) >> (p))
#else
#define ARSHIFT(n, p)                                                   \
    (((n) >> (p)) | (sam_word_t)(LSHIFT(-((n) < 0), (SAM_WORD_BIT - p))))
#endif

// Matching macro for logical shift
#define LRSHIFT(x, p)                           \
    ((x) >> (p))

// Stack access
#define POP(ptr)                                \
    HALT_IF_ERROR(sam_pop_stack(ptr))
#define PUSH(val)                               \
    HALT_IF_ERROR(sam_push_stack(val))

#define _POP_INSN(var, insn, rshift)            \
    do {                                        \
        POP(&var);                              \
        if ((var & SAM_OP_MASK) != insn)        \
            HALT(SAM_ERROR_NOT_NUMBER);         \
        var = rshift(var, SAM_OP_SHIFT);        \
    } while (0)
#define _PUSH_INSN(val, insn)                   \
    PUSH(LSHIFT((val), SAM_OP_SHIFT) | insn)

#define _POP_INT(var, rshift)                   \
    _POP_INSN(var, SAM_INSN_LIT, rshift)
#define POP_INT(var)                            \
    _POP_INT(var, ARSHIFT)
#define POP_UINT(var)                           \
    _POP_INT(var, LRSHIFT)
#define PUSH_INT(val)                           \
    _PUSH_INSN(val, SAM_INSN_LIT)

#define POP_LINK(var)                           \
    _POP_INSN(var, SAM_INSN_LINK, LRSHIFT)
#define PUSH_LINK(addr)                         \
    _PUSH_INSN(addr, SAM_INSN_LINK)

// Execution
#define DO(addr)                                \
    do {                                        \
        PUSH_LINK(pc0);                         \
        PUSH_LINK(pc);                          \
        pc0 = pc = addr;                        \
    } while (0)
#define RET                                     \
    do {                                        \
        POP_LINK(pc);                           \
        POP_LINK(pc0);                          \
        if (pc == 0)                            \
            HALT(SAM_ERROR_OK);                 \
    } while (0)

// Traps
sam_word_t trap(sam_word_t code);
sam_word_t trap_tildav2(sam_uword_t function);
