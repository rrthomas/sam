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
    if ((sam_word_t)(var & (mask)) != (insn))   \
        HALT(SAM_ERROR_WRONG_TYPE);

#define PEEK_WORD(ptr, pos)                             \
    HALT_IF_ERROR(sam_array_peek(state->s0, pos, ptr))
#define POP_WORD(ptr)                           \
    HALT_IF_ERROR(sam_array_pop(state->s0, ptr))
#define PUSH_WORD(val)                          \
    HALT_IF_ERROR(sam_array_push(state->s0, val))

#define EXTRACT_INSN(var, insn, insn_mask, rshift, shift)  \
    CHECK_TYPE(var, insn_mask, insn);                      \
    var = rshift(var, shift);                              \

#define _PEEK_INSN(var, pos, insn, insn_mask, rshift, shift)    \
    do {                                                        \
        PEEK_WORD((sam_uword_t *)&var, pos);                    \
        EXTRACT_INSN(var, insn, insn_mask, rshift, shift);      \
    } while (0)

#define _POP_INSN(var, insn, insn_mask, rshift, shift)          \
    do {                                                        \
        POP_WORD((sam_word_t *)&var);                           \
        EXTRACT_INSN(var, insn, insn_mask, rshift, shift);      \
    } while (0)

#define _POP_INT(var, rshift)                   \
    _POP_INSN(var, SAM_INT_TAG, SAM_INT_TAG_MASK, rshift, SAM_INT_SHIFT)
#define POP_INT(var)                            \
    _POP_INT(var, ARSHIFT)
#define POP_UINT(var)                           \
    _POP_INT(var, LRSHIFT)
#define PUSH_INT(val)                           \
    PUSH_WORD(SAM_INT_TAG | LSHIFT(val, SAM_INT_SHIFT))

#define POP_BOOL(var)                           \
    _POP_INSN(var, SAM_ATOM_TAG | (SAM_ATOM_BOOL << SAM_ATOM_TYPE_SHIFT), SAM_ATOM_TAG_MASK | SAM_ATOM_TYPE_MASK, LRSHIFT, SAM_ATOM_SHIFT)
#define PUSH_BOOL(val)                          \
    PUSH_WORD(SAM_ATOM_TAG | LSHIFT(SAM_ATOM_BOOL, SAM_ATOM_TYPE_SHIFT) | LSHIFT(val ? SAM_TRUE : SAM_FALSE, SAM_ATOM_SHIFT))

#define PEEK_BLOB(var, pos)                                             \
    do {                                                                \
        sam_uword_t _ptr;                                               \
        _PEEK_INSN(_ptr, pos, SAM_BLOB_TAG, SAM_BLOB_TAG_MASK, LRSHIFT, SAM_BLOB_SHIFT); \
        var = (void *)(_ptr << SAM_BLOB_SHIFT);                         \
    } while (0)

#define POP_BLOB(var)                                                    \
    do {                                                                \
        sam_uword_t _ptr;                                               \
        _POP_INSN(_ptr, SAM_BLOB_TAG, SAM_BLOB_TAG_MASK, LRSHIFT, SAM_BLOB_SHIFT); \
        var = (void *)(_ptr << SAM_BLOB_SHIFT);                         \
    } while (0)

#define PUSH_BLOB(addr)                                \
    PUSH_WORD(((sam_uword_t)addr) | SAM_BLOB_TAG)

#define POP_FLOAT(var)                                                  \
    do {                                                                \
        sam_word_t _w;                                                  \
        _POP_INSN(_w, SAM_FLOAT_TAG, SAM_FLOAT_TAG_MASK, LRSHIFT, SAM_FLOAT_SHIFT); \
        var = *(sam_float_t *)&_w;                                      \
    } while (0)
#define PUSH_FLOAT(val)                                        \
    do {                                                       \
        sam_word_t inst;                                       \
        HALT_IF_ERROR(sam_make_inst_float(&inst, val));        \
        HALT_IF_ERROR(sam_array_push(state->s0, inst));        \
    } while (0)
