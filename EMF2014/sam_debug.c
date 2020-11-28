#ifdef SAM_DEBUG

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"

void debug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

static char *disas(sam_uword_t *addr)
{
    char *text;
    sam_word_t inst;
    assert(sam_stack_peek((*addr)++, (sam_uword_t *)&inst) == SAM_ERROR_OK);
    switch (inst & SAM_OP_MASK) {
    case SAM_INSN_NOP:
        asprintf(&text, "nop");
        break;
    case SAM_INSN_INT:
        asprintf(&text, "int %d", ARSHIFT(inst, SAM_OP_SHIFT));
        break;
    case SAM_INSN_FLOAT:
        {
            sam_uword_t w2;
            assert(sam_stack_peek((*addr)++, &w2) == SAM_ERROR_OK);
            if ((w2 & SAM_OP_MASK) == SAM_INSN__FLOAT) {
                sam_uword_t f = (inst & ~SAM_OP_MASK) | ((w2 >> SAM_OP_SHIFT) & SAM_OP_MASK);
                asprintf(&text, "float %f", *(sam_float_t *)&f);
                break;
            }
        }
        // FALLTHROUGH
    case SAM_INSN__FLOAT:
        asprintf(&text, "*** UNPAIRED FLOAT ***");
        break;
    case SAM_INSN_I2F:
        asprintf(&text, "i2f");
        break;
    case SAM_INSN_F2I:
        asprintf(&text, "f2i");
        break;
    case SAM_INSN_PUSH:
        {
            sam_uword_t w2;
            assert(sam_stack_peek((*addr)++, &w2) == SAM_ERROR_OK);
            if ((w2 & SAM_OP_MASK) == SAM_INSN__PUSH) {
                asprintf(&text, "push 0x%x", ((inst & ~SAM_OP_MASK) |
                                               ((w2 >> SAM_OP_SHIFT) & SAM_OP_MASK)));
                break;
            }
        }
        // FALLTHROUGH
    case SAM_INSN__PUSH:
        asprintf(&text, "*** UNPAIRED PUSH ***");
        break;
    case SAM_INSN_POP:
        asprintf(&text, "pop");
        break;
    case SAM_INSN_DUP:
        asprintf(&text, "dup");
        break;
    case SAM_INSN_SWAP:
        asprintf(&text, "swap");
        break;
    case SAM_INSN_IDUP:
        asprintf(&text, "idup");
        break;
    case SAM_INSN_ISET:
        asprintf(&text, "iset");
        break;
    case SAM_INSN_BRA:
        asprintf(&text, "*** UNEXPECTED BRA ***");
        break;
    case SAM_INSN_KET:
        asprintf(&text, "*** UNEXPECTED KET ***");
        break;
    case SAM_INSN_LINK:
        asprintf(&text, "link %d", inst >> SAM_OP_SHIFT);
        break;
    case SAM_INSN_DO:
        asprintf(&text, "do");
        break;
    case SAM_INSN_IF:
        asprintf(&text, "if");
        break;
    case SAM_INSN_WHILE:
        asprintf(&text, "while");
        break;
    case SAM_INSN_LOOP:
        asprintf(&text, "loop");
        break;
    case SAM_INSN_NOT:
        asprintf(&text, "not");
        break;
    case SAM_INSN_AND:
        asprintf(&text, "and");
        break;
    case SAM_INSN_OR:
        asprintf(&text, "or");
        break;
    case SAM_INSN_XOR:
        asprintf(&text, "xor");
        break;
    case SAM_INSN_LSH:
        asprintf(&text, "lsh");
        break;
    case SAM_INSN_RSH:
        asprintf(&text, "rsh");
        break;
    case SAM_INSN_ARSH:
        asprintf(&text, "arsh");
        break;
    case SAM_INSN_EQ:
        asprintf(&text, "eq");
        break;
    case SAM_INSN_LT:
        asprintf(&text, "lt");
        break;
    case SAM_INSN_NEG:
        asprintf(&text, "neg");
        break;
    case SAM_INSN_ADD:
        asprintf(&text, "add");
        break;
    case SAM_INSN_MUL:
        asprintf(&text, "mul");
        break;
    case SAM_INSN_DIV:
        asprintf(&text, "div");
        break;
    case SAM_INSN_REM:
        asprintf(&text, "rem");
        break;
    case SAM_INSN_POW:
        asprintf(&text, "pow");
        break;
    case SAM_INSN_SIN:
        asprintf(&text, "sin");
        break;
    case SAM_INSN_COS:
        asprintf(&text, "cos");
        break;
    case SAM_INSN_DEG:
        asprintf(&text, "deg");
        break;
    case SAM_INSN_RAD:
        asprintf(&text, "rad");
        break;
    case SAM_INSN_HALT:
        asprintf(&text, "halt");
        break;
    case SAM_INSN_TRAP:
        asprintf(&text, "trap %d", inst >> SAM_OP_SHIFT);
        break;
    default:
        asprintf(&text, "*** INVALID OPCODE ***");
        break;
    }
    return text;
}

static void print_disas(sam_uword_t level, const char *s)
{
    sam_uword_t i;
    for (i = 0; i < level * 2; i++)
        putc(' ', stderr);
    debug("- %s\n", s);
}

static void print_stack(sam_uword_t from, sam_uword_t to)
{
    sam_uword_t i, level = 0;
    for (i = from; i < to; ) {
        sam_uword_t opcode;
        assert(sam_stack_peek(i, &opcode) == SAM_ERROR_OK);
        opcode &= SAM_OP_MASK;
        if (opcode == SAM_INSN_BRA) {
            print_disas(level, "");
            level++;
            i++;
        } else if (opcode == SAM_INSN_KET) {
            level--;
            i++;
        } else {
            char *text = disas(&i);
            print_disas(level, text);
            free(text);
        }
    }
}

void sam_print_stack(void)
{
    debug("Stack: (%u word(s))\n", sam_sp);
    print_stack(0, sam_sp);
}

void sam_print_working_stack(void)
{
    sam_uword_t static_len = sam_program_len - 2; // Subtract the initial return values.
    debug("Working stack: (%u word(s))\n", sam_sp - static_len);
    print_stack(static_len, sam_sp);
}

/* Dump screen as a PBM */
void sam_dump_screen(void)
{
    debug("P1\n");
    debug("# SAM screen dump\n");
    debug("%u %u\n", SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT);
    for (size_t j = 0; j < SAM_DISPLAY_HEIGHT; j++) {
        for (size_t i = 0; i < SAM_DISPLAY_WIDTH; i++) {
            debug("%d", sam_getpixel(i, j) ? 0 : 1);
            if (i < SAM_DISPLAY_WIDTH - 1)
                debug(" ");
        }
        debug("\n");
    }
}

#endif
