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
        sam_word_t _err = code;                 \
        if (_err != SAM_ERROR_OK)               \
            HALT(_err);                         \
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
#define POP_WORD(ptr)                           \
    HALT_IF_ERROR(sam_pop_stack(ptr))
#define PUSH_WORD(val)                          \
    HALT_IF_ERROR(sam_push_stack(val))

#define _POP_INSN(var, insn, rshift, err)       \
    do {                                        \
        POP_WORD((sam_word_t *)&var);           \
        if ((var & SAM_OP_MASK) != insn)        \
            HALT(err);                          \
        var = rshift(var, SAM_OP_SHIFT);        \
    } while (0)
#define _PUSH_INSN(val, insn)                           \
    PUSH_WORD(LSHIFT((val), SAM_OP_SHIFT) | insn)

#define _POP_INT(var, rshift)                   \
    _POP_INSN(var, SAM_INSN_INT, rshift, SAM_ERROR_NOT_INT)
#define POP_INT(var)                            \
    _POP_INT(var, ARSHIFT)
#define POP_UINT(var)                           \
    _POP_INT(var, LRSHIFT)
#define PUSH_INT(val)                           \
    _PUSH_INSN(val, SAM_INSN_INT)

#define POP_LINK(var)                           \
    _POP_INSN(var, SAM_INSN_LINK, LRSHIFT, SAM_ERROR_NOT_CODE)
#define PUSH_LINK(addr)                         \
    _PUSH_INSN(addr, SAM_INSN_LINK)

#define POP_FLOAT(var)                                                  \
    do {                                                                \
        sam_uword_t w2;                                                 \
        POP_WORD((sam_word_t *)&w2);                                    \
        if ((w2 & SAM_OP_MASK) != SAM_INSN__FLOAT)                      \
            HALT(SAM_ERROR_NOT_FLOAT);                                  \
        sam_uword_t w1;                                                 \
        POP_WORD((sam_word_t *)&w1);                                    \
        if ((w1 & SAM_OP_MASK) != SAM_INSN_FLOAT)                       \
            HALT(SAM_ERROR_UNPAIRED_FLOAT);                             \
        w1 = ((w1 & ~SAM_OP_MASK) | ((w2 >> SAM_OP_SHIFT) & SAM_OP_MASK)); \
        var = *(sam_float_t *)&w1;                                      \
    } while (0)

#define PUSH_FLOAT(val)                                                 \
    do {                                                                \
        sam_float_t v = val;                                            \
        _PUSH_INSN((*(sam_uword_t *)&v >> SAM_OP_SHIFT), SAM_INSN_FLOAT); \
        _PUSH_INSN((*(sam_uword_t *)&v & SAM_OP_MASK), SAM_INSN__FLOAT); \
    } while (0)

// Execution
#define DO(addr)                                \
    do {                                        \
        PUSH_LINK(sam_pc0);                     \
        PUSH_LINK(pc);                          \
        sam_pc0 = pc = addr;                    \
    } while (0)
#define RET                                     \
    do {                                        \
        POP_LINK(pc);                           \
        POP_LINK(sam_pc0);                      \
    } while (0)

// Program
#define SAM_STACK_WORDS 4096
extern sam_word_t sam_stack[SAM_STACK_WORDS];
extern sam_uword_t sam_program_len;

// Traps
sam_word_t trap(sam_word_t code);
sam_word_t trap_tildav2(sam_uword_t function);
