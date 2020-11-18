#include <stdio.h>
#include <stdlib.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"

static char *disas(sam_word_t *addr)
{
    char *text;
    sam_word_t inst = sam_s0[(*addr)++];
    switch (inst & SAM_OP_MASK) {
    case SAM_INSN_NOP:
        asprintf(&text, "nop");
        break;
    case SAM_INSN_INT:
        asprintf(&text, "int %d", ARSHIFT(inst, SAM_OP_SHIFT));
        break;
    case SAM_INSN_FLOAT:
        {
            sam_uword_t w2 = sam_s0[(*addr)++];
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
            sam_uword_t w2 = sam_s0[(*addr)++];
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
    case SAM_INSN_SET:
        asprintf(&text, "set");
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
    fprintf(stderr, "- %s\n", s);
}

void sam_print_stack(void)
{
    fprintf(stderr, "Stack: (%u words)\n", sam_sp);
    sam_uword_t i, level = 0;
    for (i = 0; i < sam_sp; ) {
        sam_uword_t opcode = sam_s0[i] & SAM_OP_MASK;
        if (opcode == SAM_INSN_BRA) {
            print_disas(level, "");
            level++;
            i++;
        } else if (opcode == SAM_INSN_KET) {
            level--;
            i++;
        }
        else {
            char *text = disas(&i);
            print_disas(level, text);
            free(text);
        }
    }
}
