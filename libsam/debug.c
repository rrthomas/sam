// SAM debug functions.
//
// (c) Reuben Thomas 1994-2026
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifdef SAM_DEBUG

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "private.h"
#include "traps_basic.h"
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

static void xstrcat(char **s, char *t)
{
    char *res;
    xasprintf(&res, "%s%s", *s, t);
    free(*s);
    free(t);
    *s = res;
}

char *inst_name(sam_uword_t inst_opcode)
{
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
    case INST_IGET:
        return "iget";
    case INST_ISET:
        return "iset";
    case INST_IPOP:
        return "ipop";
    case INST_IPUSH:
        return "ipush";
    case INST_GO:
        return "go";
    case INST_DO:
        return "do";
    case INST_CALL:
        return "call";
    case INST_IF:
        return "if";
    case INST_WHILE:
        return "while";
    case INST_NOT:
        return "not";
    case INST_AND:
        return "and";
    case INST_OR:
        return "or";
    case INST_XOR:
        return "xor";
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
    case SAM_TRAP_BASIC_BASE:
        name = sam_basic_trap_name(function);
        break;
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

typedef struct sam_stack_list {
    sam_stack_t *stack;
    struct sam_stack_list *next;
} sam_stack_list_t;

static sam_stack_list_t *list_append(sam_stack_list_t *l, sam_stack_t *s)
{
    sam_stack_list_t *node = malloc(sizeof(struct sam_stack_list));
    assert(node != NULL);
    node->stack = s;
    node->next = l;
    return node;
}

static void free_list(sam_stack_list_t *l)
{
    sam_stack_list_t *next;
    for (sam_stack_list_t *p = l; p != NULL; p = next) {
        next = p->next;
        free(p);
    }
}

static char *indent(sam_uword_t level, char *s)
{
    char *res, *new_s;
    xasprintf(&res, "");
    for (sam_uword_t i = 0; i < level; i++) {
        xasprintf(&new_s, "%s%s", res, "  ");
        free(res);
        res = new_s;
    }
    xasprintf(&new_s, "%s- %s\n", res, s);
    free(res);
    return new_s;
}

static bool already_visited(sam_stack_list_t *l, sam_stack_t *s)
{
    for (sam_stack_list_t *p = l; p != NULL; p = p->next) {
        if (p->stack == s)
            return true;
    }
    return false;
}

static sam_stack_list_t *disas_stack(sam_stack_list_t *l, sam_uword_t level, sam_stack_t *s, sam_uword_t from, sam_uword_t to, char **text)
{
    xasprintf(text, "");
    for (sam_uword_t i = from; i < to; i++) {
        sam_uword_t opcode;
        assert(sam_stack_peek(s, i, &opcode) == SAM_ERROR_OK);
        if ((opcode & SAM_STACK_TAG_MASK) == SAM_STACK_TAG) {
            sam_uword_t *addr = (sam_uword_t *)(opcode & ~SAM_STACK_TAG_MASK);
            sam_stack_t *inner_s = (sam_stack_t *)addr;
            if (l == NULL || already_visited(l, inner_s)) {
                char *stack_str;
                xasprintf(&stack_str, "stack %p", inner_s);
                char *line = indent(level, stack_str);
                xstrcat(text, line);
            } else {
                char *count_str;
                xasprintf(&count_str, "stack %p (%u items)", inner_s, inner_s->sp);
                char *line = indent(level, count_str);
                xstrcat(text, line);
                free(count_str);
                l = list_append(l, inner_s);
                char *stack_str;
                l = disas_stack(l, level + 1, inner_s, 0, inner_s->sp, &stack_str);
                xstrcat(text, stack_str);
            }
        } else {
            sam_word_t inst;
            assert(sam_stack_peek(s, i, (sam_uword_t *)&inst) == SAM_ERROR_OK);
            char *inst_str = disas(inst);
            char *line = indent(level, inst_str);
            free(inst_str);
            xstrcat(text, line);
        }
    }
    return l;
}

char *disas(sam_word_t inst)
{
    char *text = NULL;
    if ((inst & SAM_INT_TAG_MASK) == SAM_INT_TAG) {
        xasprintf(&text, "int %zd", ARSHIFT(inst, SAM_INT_SHIFT));
    } else if ((inst & SAM_FLOAT_TAG_MASK) == SAM_FLOAT_TAG) {
        sam_uword_t operand = inst >> SAM_FLOAT_SHIFT;
        xasprintf(&text, "float %f", *(sam_float_t *)&operand);
    } else if ((inst & SAM_ATOM_TAG_MASK) == SAM_ATOM_TAG) {
        // No atoms yet.
        switch ((inst & SAM_ATOM_TYPE_MASK) >> SAM_ATOM_TYPE_SHIFT) {}
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
    } else if ((inst & SAM_STACK_TAG_MASK) == SAM_STACK_TAG) {
        sam_stack_t *s = (sam_stack_t *)(inst & ~SAM_STACK_TAG_MASK);
        sam_stack_list_t *l = list_append(NULL, s);
        free(disas_stack(l, 0, s, 0, s->sp, &text));
    } else {
        abort(); // The cases are exhaustive.
    }
    return text;
}

void sam_print_stack(sam_stack_t *s)
{
    debug("Stack: %p (%u item(s))\n", s, s->sp);
    sam_stack_list_t *l = list_append(NULL, s);
    char *text;
    l = disas_stack(l, 0, s, 0, s->sp, &text);
    debug("%s", text);
    free(text);
    free_list(l);
}

void sam_print_working_stack(sam_stack_t *s)
{
    debug("Working stack: (%u word(s))\n", s->sp);
    char *text;
    disas_stack(NULL, 0, s, 0, s->sp, &text);
    debug("%s", text);
    free(text);
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
int sam_debug_init(sam_state_t *state)
{
    return 0;
}

#endif
