// SAM debug functions.
//
// (c) Reuben Thomas 1994-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifdef SAM_DEBUG

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "private.h"
#include "traps_math.h"
#include "traps_graphics.h"

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

char *inst_name(sam_uword_t inst_opcode) {
    switch (inst_opcode) {
    case INST_NOP:
        return "nop";
    case INST_POP:
        return "pop";
    case INST_GET:
        return "get";
    case INST_SET:
        return "set";
    case INST_EXTRACT:
        return "extract";
    case INST_INSERT:
        return "insert";
    case INST_DO:
        return "do";
    case INST_IF:
        return "if";
    case INST_WHILE:
        return "while";
    case INST_LOOP:
        return "loop";
    case INST_NOT:
        return "not";
    case INST_AND:
        return "and";
    case INST_OR:
        return "or";
    case INST_XOR:
        return "xor";
    case INST_LSH:
        return "lsh";
    case INST_RSH:
        return "rsh";
    case INST_ARSH:
        return "arsh";
    case INST_EQ:
        return "eq";
    case INST_LT:
        return "lt";
    case INST_NEG:
        return "neg";
    case INST_ADD:
        return "add";
    case INST_MUL:
        return "mul";
    case INST_DIV:
        return "div";
    case INST_REM:
        return "rem";
    case INST_I2F:
        return "i2f";
    case INST_F2I:
        return "f2i";
    case INST_0:
        return "zero";
    case INST_1:
        return "one";
    case INST_MINUS_1:
        return "_one";
    case INST_2:
        return "two";
    case INST_MINUS_2:
        return "_two";
    case INST_HALT:
        return "halt";
    default:
        return "INVALID INSTRUCTION";
    }
}

char *trap_name(sam_uword_t function)
{
    char *name = NULL;
    switch (function & SAM_TRAP_BASE_MASK) {
    case SAM_TRAP_MATH_BASE:
        name = sam_math_trap_name(function);
        break;
    case SAM_TRAP_GRAPHICS_BASE:
        name = sam_graphics_trap_name(function);
        break;
    default:
        break;
    }
    if (name != NULL)
        return name;
    else {
        static char buf[21]; // 20 digits in 64-bit SIZE_MAX + NUL
        sprintf(buf, "%zu", function);
        return buf;
    }
}

static char *disas(sam_uword_t *addr)
{
    char *text = NULL;
    sam_word_t inst;
    assert(sam_stack_peek(sam_stack, (*addr)++, (sam_uword_t *)&inst) == SAM_ERROR_OK);
    if ((inst & SAM_REF_TAG_MASK) == SAM_REF_TAG) {
        xasprintf(&text, "ref %zd", inst >> SAM_REF_SHIFT);
    } else if ((inst & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
        xasprintf(&text, "int %zd", ARSHIFT(inst, SAM_INT_SHIFT));
    } else if ((inst & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
        uint64_t operand = (uint64_t)(inst >> SAM_FLOAT_SHIFT);
        xasprintf(&text, "float %f", *(sam_float_t *)&operand);
    } else if ((inst & SAM_ATOM_TAG_MASK) == SAM_ATOM_TAG) {
        // No atoms yet.
        switch ((inst & SAM_ATOM_TYPE_MASK) >> SAM_ATOM_TYPE_SHIFT) {}
    } else if ((inst & SAM_ARRAY_TAG_MASK) == SAM_ARRAY_TAG) {
        switch ((inst & SAM_ARRAY_TYPE_MASK) >> SAM_ARRAY_TYPE_SHIFT) {
        case SAM_ARRAY_STACK:
            xasprintf(&text, "*** UNEXPECTED %s ***", ARSHIFT(inst, SAM_ARRAY_OFFSET_SHIFT) > 0 ? "BRA" : "KET");
            break;
        // TODO:
        /* case SAM_ARRAY_RAW: */
        /*     break; */
        }
    } else if ((inst & SAM_TRAP_TAG_MASK) == SAM_TRAP_TAG) {
        sam_uword_t function = inst >> SAM_TRAP_FUNCTION_SHIFT;
        xasprintf(&text, "trap %s", trap_name(function));
    } else if ((inst & SAM_INSTS_TAG_MASK) == SAM_INSTS_TAG) {
        sam_uword_t opcodes = (sam_uword_t)inst >> SAM_INSTS_SHIFT;
        do {
            sam_word_t opcode = opcodes & SAM_INST_MASK;
            if (text == NULL)
                xasprintf(&text, "%s", inst_name(opcode));
            else
                xasprintf(&text, "%s %s", text, inst_name(opcode));
            opcodes >>= SAM_INST_SHIFT;
        } while (opcodes != 0);
    } else {
        abort(); // The cases are exhaustive.
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
        assert(sam_stack_peek(sam_stack, i, &opcode) == SAM_ERROR_OK);
        if ((opcode & SAM_ARRAY_TAG_MASK) == SAM_ARRAY_TAG) {
            sam_word_t offset = ARSHIFT((sam_word_t)opcode, SAM_ARRAY_OFFSET_SHIFT);
            if (offset > 0) {
                print_disas(level, "");
                level++;
                i++;
            } else if (offset < 0) {
                level--;
                i++;
            } else
                print_disas(level, "ARRAY with unexpected zero offset");
        } else {
            char *text = disas(&i);
            print_disas(level, text);
            free(text);
        }
    }
}

void sam_print_stack(void)
{
    debug("Stack: (%u word(s))\n", sam_stack->sp);
    print_stack(0, sam_stack->sp);
}

void sam_print_working_stack(void)
{
    debug("Working stack: (%u word(s))\n", sam_stack->sp - sam_program_len);
    print_stack(sam_program_len, sam_stack->sp);
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

// Initialise debug state.
int sam_debug_init()
{
    sam_program_len = sam_stack->sp;

    return 0;
}

#endif
