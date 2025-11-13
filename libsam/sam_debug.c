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

static void xasprintf(char **text, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    assert(vasprintf(text, fmt, ap) != -1);
    va_end(ap);
}

char *trap_name(sam_word_t trap_code) {
    switch (trap_code) {
        case TRAP_NOP:
            return "nop";
        case TRAP_I2F:
            return "i2f";
        case TRAP_F2I:
            return "f2i";
        case TRAP_POP:
            return "pop";
        case TRAP_GET:
            return "get";
        case TRAP_SET:
            return "set";
        case TRAP_IGET:
            return "iget";
        case TRAP_ISET:
            return "iset";
        case TRAP_DO:
            return "do";
        case TRAP_IF:
            return "if";
        case TRAP_WHILE:
            return "while";
        case TRAP_LOOP:
            return "loop";
        case TRAP_NOT:
            return "not";
        case TRAP_AND:
            return "and";
        case TRAP_OR:
            return "or";
        case TRAP_XOR:
            return "xor";
        case TRAP_LSH:
            return "lsh";
        case TRAP_RSH:
            return "rsh";
        case TRAP_ARSH:
            return "arsh";
        case TRAP_EQ:
            return "eq";
        case TRAP_LT:
            return "lt";
        case TRAP_NEG:
            return "neg";
        case TRAP_ADD:
            return "add";
        case TRAP_MUL:
            return "mul";
        case TRAP_DIV:
            return "div";
        case TRAP_REM:
            return "rem";
        case TRAP_POW:
            return "pow";
        case TRAP_SIN:
            return "sin";
        case TRAP_COS:
            return "cos";
        case TRAP_DEG:
            return "deg";
        case TRAP_RAD:
            return "rad";
        case TRAP_HALT:
            return "halt";
        default:
        {
            char *text;
            xasprintf(&text, "trap %d", trap_code);
            return text;
        }
    }
}

static char *disas(sam_uword_t *addr)
{
    char *text;
    sam_word_t inst;
    assert(sam_stack_peek((*addr)++, (sam_uword_t *)&inst) == SAM_ERROR_OK);
    switch (inst & SAM_OP_MASK) {
    case SAM_INSN_TRAP:
        xasprintf(&text, "%s", trap_name(inst >> SAM_OP_SHIFT));
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
    case SAM_INSN_STACK:
        xasprintf(&text, "*** UNEXPECTED %s ***", ARSHIFT(inst, SAM_OP_SHIFT) > 0 ? "BRA" : "KET");
        break;
    case SAM_INSN_LINK:
        xasprintf(&text, "link %d", inst >> SAM_OP_SHIFT);
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
        sam_word_t operand = ARSHIFT((sam_word_t)opcode, SAM_OP_SHIFT);
        opcode &= SAM_OP_MASK;
        if (opcode == SAM_INSN_STACK && operand > 0) {
            print_disas(level, "");
            level++;
            i++;
        } else if (opcode == SAM_INSN_STACK && operand < 0) {
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
