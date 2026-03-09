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

#define XEXTRACT_BLOB(blob, type_code, blob_type, var)  \
    do {                                                \
        assert(blob->type == type_code);                \
        var = (blob_type *)(&(blob->data));             \
    } while (0)

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
    case INST_NEW:
        return "new";
    case INST_S0:
        return "s0";
    case INST_DROP:
        return "drop";
    case INST_GET:
        return "get";
    case INST_SET:
        return "set";
    case INST_EXTRACT:
        return "extract";
    case INST_INSERT:
        return "insert";
    case INST_POP:
        return "pop";
    case INST_SHIFT:
        return "shift";
    case INST_APPEND:
        return "append";
    case INST_PREPEND:
        return "prepend";
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

typedef struct sam_blob_list {
    sam_blob_t *blob;
    struct sam_blob_list *next;
} sam_blob_list_t;

static sam_blob_list_t *list_append(sam_blob_list_t *l, sam_blob_t *blob)
{
    sam_blob_list_t *node = malloc(sizeof(struct sam_blob_list));
    assert(node != NULL);
    node->blob = blob;
    node->next = l;
    return node;
}

static void free_list(sam_blob_list_t *l)
{
    sam_blob_list_t *next;
    for (sam_blob_list_t *p = l; p != NULL; p = next) {
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

static bool already_visited(sam_blob_list_t *l, sam_blob_t *blob)
{
    for (sam_blob_list_t *p = l; p != NULL; p = p->next) {
        if (p->blob == blob)
            return true;
    }
    return false;
}

static sam_blob_list_t *disas_word(sam_blob_list_t *l, sam_uword_t level, sam_word_t opcode, char **text);

static sam_blob_list_t *disas_stack(sam_blob_list_t *l, sam_uword_t level, sam_blob_t *blob, char **text)
{
    xasprintf(text, "");
    sam_stack_t *s;
    XEXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    for (sam_uword_t i = 0; i < s->sp; i++) {
        sam_uword_t opcode;
        assert(sam_stack_peek(blob, i, &opcode) == SAM_ERROR_OK);
        l = disas_word(l, level, opcode, text);
    }
    return l;
}

static sam_blob_list_t *disas_map(sam_blob_list_t *l, sam_uword_t level, sam_blob_t *blob, char **text)
{
    xasprintf(text, "");
    sam_blob_t *iter_blob;
    assert(sam_map_iter_new(blob, &iter_blob) == SAM_ERROR_OK);
    sam_iter_t *i;
    XEXTRACT_BLOB(iter_blob, SAM_BLOB_ITER, sam_iter_t, i);
    for (;;) {
        sam_word_t key, val;
        assert(i->next(i, &key) == SAM_ERROR_OK);
        if (key == SAM_VALUE_NULL)
            break;
        assert(sam_map_get(blob, key, &val) == SAM_ERROR_OK);
        l = disas_word(l, level, key, text);
        l = disas_word(l, level + 1, val, text);
    }
    return l;
}

static sam_blob_list_t *disas_word(sam_blob_list_t *l, sam_uword_t level, sam_word_t opcode, char **text)
{
    if ((opcode & SAM_BLOB_TAG_MASK) == SAM_BLOB_TAG) {
        sam_blob_t *inner_blob = (sam_blob_t *)(opcode & ~SAM_BLOB_TAG_MASK);
        char *blob_str;
        switch (inner_blob->type) {
        case SAM_BLOB_STACK:
            {
                sam_stack_t *inner_s;
                XEXTRACT_BLOB(inner_blob, SAM_BLOB_STACK, sam_stack_t, inner_s);
                xasprintf(&blob_str, "stack %p (%u items)", inner_s, inner_s->sp);
            }
            break;
        case SAM_BLOB_MAP:
            {
                sam_map_t *inner_m;
                XEXTRACT_BLOB(inner_blob, SAM_BLOB_MAP, sam_map_t, inner_m);
                xasprintf(&blob_str, "map %p", inner_m);
            }
            break;
        case SAM_BLOB_ITER:
            {
                sam_iter_t *inner_i;
                XEXTRACT_BLOB(inner_blob, SAM_BLOB_ITER, sam_iter_t, inner_i);
                xasprintf(&blob_str, "iter %p", inner_i);
            }
            break;
        case SAM_BLOB_STRING:
            {
                sam_string_t *inner_str;
                XEXTRACT_BLOB(inner_blob, SAM_BLOB_STRING, sam_string_t, inner_str);
                xasprintf(&blob_str, "string %s", inner_str->str);
            }
        }
        char *line = indent(level, blob_str);
        xstrcat(text, line);
        if (l != NULL && !already_visited(l, inner_blob)) {
            l = list_append(l, inner_blob);
            char *blob_str;
            switch (inner_blob->type) {
            case SAM_BLOB_STACK:
                l = disas_stack(l, level + 1, inner_blob, &blob_str);
                xstrcat(text, blob_str);
                break;
            case SAM_BLOB_MAP:
                l = disas_map(l, level + 1, inner_blob, &blob_str);
                xstrcat(text, blob_str);
                break;
            }
        }
    } else {
        char *inst_str = disas(opcode);
        char *line = indent(level, inst_str);
        free(inst_str);
        xstrcat(text, line);
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
        sam_word_t atom_type = (inst & SAM_ATOM_TYPE_MASK) >> SAM_ATOM_TYPE_SHIFT;
        switch (atom_type) {
        case SAM_ATOM_NULL:
            xasprintf(&text, "null");
            break;
        default:
            xasprintf(&text, "invalid atom type %d", atom_type);
            break;
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
            opcodes >>= SAM_ONE_INST_SHIFT;
        } while (opcodes != 0);
    } else if ((inst & SAM_BLOB_TAG_MASK) == SAM_BLOB_TAG) {
        sam_blob_t *blob = (sam_blob_t *)(inst & ~SAM_BLOB_TAG_MASK);
        sam_blob_list_t *l = list_append(NULL, blob);
        switch (blob->type) {
        case SAM_BLOB_STACK:
            free(disas_stack(l, 0, blob, &text));
            break;
        case SAM_BLOB_MAP:
            free(disas_map(l, 0, blob, &text));
            break;
        }
    } else {
        abort(); // The cases are exhaustive.
    }
    return text;
}

void sam_print_stack(sam_blob_t *blob)
{
    sam_stack_t *s;
    XEXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    debug("Stack: %p (%u item(s))\n", blob, s->sp);
    sam_blob_list_t *l = list_append(NULL, blob);
    char *text;
    l = disas_stack(l, 0, blob, &text);
    debug("%s", text);
    free(text);
    free_list(l);
}

void sam_print_working_stack(sam_blob_t *blob)
{
    sam_stack_t *s;
    XEXTRACT_BLOB(blob, SAM_BLOB_STACK, sam_stack_t, s);
    debug("Working stack: (%u word(s))\n", s->sp);
    char *text;
    disas_stack(NULL, 0, blob, &text);
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

#endif
