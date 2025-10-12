#ifdef SAM_DEBUG

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"

bool do_debug = false;

void debug(const char *fmt, ...)
{
    if (do_debug) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
}

void xasprintf(char **text, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    assert(vasprintf(text, fmt, ap) != -1);
    va_end(ap);
}

static char *disas(sam_uword_t *addr)
{
    char *text;
    sam_word_t inst;
    assert(sam_stack_peek((*addr)++, (sam_uword_t *)&inst) == SAM_ERROR_OK);
    switch (inst & SAM_OP_MASK) {
    case SAM_INSN_NOP:
        xasprintf(&text, "nop");
        break;
    case SAM_INSN_INT:
        xasprintf(&text, "int %d", ARSHIFT(inst, SAM_OP_SHIFT));
        break;
    case SAM_INSN_FLOAT:
        {
            sam_uword_t w2;
            assert(sam_stack_peek((*addr)++, &w2) == SAM_ERROR_OK);
            if ((w2 & SAM_OP_MASK) == SAM_INSN__FLOAT) {
                sam_uword_t f = (inst & ~SAM_OP_MASK) | ((w2 >> SAM_OP_SHIFT) & SAM_OP_MASK);
                xasprintf(&text, "float %f", *(sam_float_t *)&f);
                break;
            }
        }
        // FALLTHROUGH
    case SAM_INSN__FLOAT:
        xasprintf(&text, "*** UNPAIRED FLOAT ***");
        break;
    case SAM_INSN_I2F:
        xasprintf(&text, "i2f");
        break;
    case SAM_INSN_F2I:
        xasprintf(&text, "f2i");
        break;
    case SAM_INSN_PUSH:
        {
            sam_uword_t w2;
            assert(sam_stack_peek((*addr)++, &w2) == SAM_ERROR_OK);
            if ((w2 & SAM_OP_MASK) == SAM_INSN__PUSH) {
                xasprintf(&text, "push 0x%x", ((inst & ~SAM_OP_MASK) |
                                               ((w2 >> SAM_OP_SHIFT) & SAM_OP_MASK)));
                break;
            }
        }
        // FALLTHROUGH
    case SAM_INSN__PUSH:
        xasprintf(&text, "*** UNPAIRED PUSH ***");
        break;
    case SAM_INSN_POP:
        xasprintf(&text, "pop");
        break;
    case SAM_INSN_DUP:
        xasprintf(&text, "dup");
        break;
    case SAM_INSN_SWAP:
        xasprintf(&text, "swap");
        break;
    case SAM_INSN_IDUP:
        xasprintf(&text, "idup");
        break;
    case SAM_INSN_ISET:
        xasprintf(&text, "iset");
        break;
    case SAM_INSN_BRA:
        xasprintf(&text, "*** UNEXPECTED BRA ***");
        break;
    case SAM_INSN_KET:
        xasprintf(&text, "*** UNEXPECTED KET ***");
        break;
    case SAM_INSN_LINK:
        xasprintf(&text, "link %d", inst >> SAM_OP_SHIFT);
        break;
    case SAM_INSN_DO:
        xasprintf(&text, "do");
        break;
    case SAM_INSN_IF:
        xasprintf(&text, "if");
        break;
    case SAM_INSN_WHILE:
        xasprintf(&text, "while");
        break;
    case SAM_INSN_LOOP:
        xasprintf(&text, "loop");
        break;
    case SAM_INSN_NOT:
        xasprintf(&text, "not");
        break;
    case SAM_INSN_AND:
        xasprintf(&text, "and");
        break;
    case SAM_INSN_OR:
        xasprintf(&text, "or");
        break;
    case SAM_INSN_XOR:
        xasprintf(&text, "xor");
        break;
    case SAM_INSN_LSH:
        xasprintf(&text, "lsh");
        break;
    case SAM_INSN_RSH:
        xasprintf(&text, "rsh");
        break;
    case SAM_INSN_ARSH:
        xasprintf(&text, "arsh");
        break;
    case SAM_INSN_EQ:
        xasprintf(&text, "eq");
        break;
    case SAM_INSN_LT:
        xasprintf(&text, "lt");
        break;
    case SAM_INSN_NEG:
        xasprintf(&text, "neg");
        break;
    case SAM_INSN_ADD:
        xasprintf(&text, "add");
        break;
    case SAM_INSN_MUL:
        xasprintf(&text, "mul");
        break;
    case SAM_INSN_DIV:
        xasprintf(&text, "div");
        break;
    case SAM_INSN_REM:
        xasprintf(&text, "rem");
        break;
    case SAM_INSN_POW:
        xasprintf(&text, "pow");
        break;
    case SAM_INSN_SIN:
        xasprintf(&text, "sin");
        break;
    case SAM_INSN_COS:
        xasprintf(&text, "cos");
        break;
    case SAM_INSN_DEG:
        xasprintf(&text, "deg");
        break;
    case SAM_INSN_RAD:
        xasprintf(&text, "rad");
        break;
    case SAM_INSN_HALT:
        xasprintf(&text, "halt");
        break;
    case SAM_INSN_TRAP:
        xasprintf(&text, "trap %d", inst >> SAM_OP_SHIFT);
        break;
    default:
        xasprintf(&text, "*** INVALID OPCODE ***");
        break;
    }
    return text;
}

static void print_disas(sam_uword_t level, const char *s)
{
    sam_uword_t i;
    for (i = 0; i < level * 2; i++)
        debug(" ");
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
void sam_dump_screen(const char *filename)
{
    FILE *fp = fopen(filename, "w");
    if (fp == NULL)
        debug("could not open file %s\n", filename);
    else {
        fprintf(fp, "P1\n");
        fprintf(fp, "# SAM screen dump\n");
        fprintf(fp, "%u %u\n", SAM_DISPLAY_WIDTH, SAM_DISPLAY_HEIGHT);
        for (size_t j = 0; j < SAM_DISPLAY_HEIGHT; j++) {
            for (size_t i = 0; i < SAM_DISPLAY_WIDTH; i++) {
                fprintf(fp, "%d", sam_getpixel(i, j) ? 0 : 1);
                if (i < SAM_DISPLAY_WIDTH - 1)
                    fprintf(fp, " ");
            }
            fprintf(fp, "\n");
        }
        fclose(fp);
    }
}

#endif
