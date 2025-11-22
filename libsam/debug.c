#ifdef SAM_DEBUG

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "private.h"

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

char *trap_name(sam_word_t trap_opcode) {
    switch (trap_opcode) {
        case INST_BLACK:
            return "BLACK";
        case INST_WHITE:
            return "WHITE";
        case INST_DISPLAY_WIDTH:
            return "DISPLAY_WIDTH";
        case INST_DISPLAY_HEIGHT:
            return "DISPLAY_HEIGHT";
        case INST_CLEARSCREEN:
            return "CLEARSCREEN";
        case INST_SETDOT:
            return "SETDOT";
        case INST_DRAWLINE:
            return "DRAWLINE";
        case INST_DRAWRECT:
            return "DRAWRECT";
        case INST_DRAWROUNDRECT:
            return "DRAWROUNDRECT";
        case INST_FILLRECT:
            return "FILLRECT";
        case INST_DRAWCIRCLE:
            return "DRAWCIRCLE";
        case INST_FILLCIRCLE:
            return "FILLCIRCLE";
        case INST_DRAWBITMAP:
            return "DRAWBITMAP";
        default: {
            char *text;
            xasprintf(&text, "%zd", trap_opcode);
            return text;
        }
    }
}

char *inst_name(sam_word_t inst_opcode) {
    switch (inst_opcode) {
        case INST_NOP:
            return "nop";
        case INST_I2F:
            return "i2f";
        case INST_F2I:
            return "f2i";
        case INST_POP:
            return "pop";
        case INST_GET:
            return "get";
        case INST_SET:
            return "set";
        case INST_IGET:
            return "iget";
        case INST_ISET:
            return "iset";
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
        case INST_POW:
            return "pow";
        case INST_SIN:
            return "sin";
        case INST_COS:
            return "cos";
        case INST_DEG:
            return "deg";
        case INST_RAD:
            return "rad";
        case INST_HALT:
            return "halt";
        default:
        {
            char *text;
            xasprintf(&text, "trap %s", trap_name(inst_opcode));
            return text;
        }
    }
}

static char *disas(sam_stack_t *s, sam_uword_t addr)
{
    char *text;
    sam_word_t inst;
    assert(sam_stack_peek(s, addr, (sam_uword_t *)&inst) == SAM_ERROR_OK);
    switch (inst & SAM_TAG_MASK) {
    case SAM_TAG_LINK:
        xasprintf(&text, "link %zd", inst >> SAM_LINK_SHIFT);
        break;
    case SAM_TAG_ATOM:
        switch ((inst & SAM_ATOM_TYPE_MASK) >> SAM_ATOM_TYPE_SHIFT) {
        case SAM_ATOM_INST:
            xasprintf(&text, "%s", inst_name(inst >> SAM_OPERAND_SHIFT));
            break;
        case SAM_ATOM_INT:
            xasprintf(&text, "int %zd", ARSHIFT(inst, SAM_OPERAND_SHIFT));
            break;
        case SAM_ATOM_FLOAT: {
            sam_word_t operand = inst >> SAM_OPERAND_SHIFT;
            xasprintf(&text, "float %f", *(sam_float_t *)&operand);
            break;
        }
        /* TODO: case SAM_ATOM_CHAR: */
        }
        break;
    case SAM_TAG_ARRAY:
        switch ((inst & SAM_ARRAY_TYPE_MASK) >> SAM_ARRAY_TYPE_SHIFT) {
        case SAM_ARRAY_STACK:
            xasprintf(&text, "*** UNEXPECTED %s ***", ARSHIFT(inst, SAM_OPERAND_SHIFT) > 0 ? "BRA" : "KET");
            break;
        // TODO:
        /* case SAM_ARRAY_RAW: */
        /*     break; */
        }
        break;
    default:
        xasprintf(&text, "*** INVALID TAG ***");
    }
    return text;
}

static void print_disas(sam_uword_t level, const char *s)
{
    for (sam_uword_t i = 0; i < level * 2; i++)
        debug(" ");
    debug("- %s\n", s);
}

typedef struct sam_stack_list {
    sam_stack_t *stack;
    struct sam_stack_list *next;
} sam_stack_list_t;

static sam_stack_list_t *list_append(sam_stack_list_t *l, sam_stack_t *s) {
    sam_stack_list_t *node = malloc(sizeof(struct sam_stack_list));
    assert(node != NULL);
    node->stack = s;
    node->next = l;
    return node;
}

static void free_list(sam_stack_list_t *l) {
    sam_stack_list_t *next;
    for (sam_stack_list_t *p = l; p != NULL; p = next) {
        next = p->next;
        free(p);
    }
}

static bool already_visited(sam_stack_list_t *l, sam_stack_t *s) {
    for (sam_stack_list_t *p = l; p != NULL; p = p->next) {
        if (p->stack == s)
            return true;
    }
    return false;
}

static sam_stack_list_t *print_stack(sam_stack_list_t *l, sam_uword_t level, sam_stack_t *s, sam_uword_t from, sam_uword_t to)
{
    for (sam_uword_t i = from; i < to; i++) {
        sam_uword_t opcode;
        assert(sam_stack_peek(s, i, &opcode) == SAM_ERROR_OK);
        sam_word_t operand = ARSHIFT((sam_word_t)opcode, SAM_OPERAND_SHIFT);
        sam_word_t tag = opcode & SAM_TAG_MASK;
        if (tag == SAM_TAG_ARRAY) {
            sam_stack_t *inner_s = (sam_stack_t *)(opcode & ~SAM_TAG_MASK);
            if (l == NULL || already_visited(l, inner_s)) {
                char *stack_str;
                xasprintf(&stack_str, "stack %p", inner_s);
                print_disas(level, stack_str);
            } else {
                char *count_str;
                xasprintf(&count_str, "stack %p (%u items)", inner_s, inner_s->sp);
                print_disas(level, count_str);
                free(count_str);
                l = list_append(l, inner_s);
                l = print_stack(l, level + 1, inner_s, 0, inner_s->sp);
            }
        } else {
            char *text = disas(s, i);
            print_disas(level, text);
            free(text);
        }
    }
    return l;
}

void sam_print_stack(sam_stack_t *s)
{
    debug("Stack: %p (%u items)\n", sam_stack, sam_stack->sp);
    sam_stack_list_t *l = list_append(NULL, sam_stack);
    free_list(print_stack(l, 0, s, 0, s->sp));
}

void sam_print_working_stack(sam_stack_t *s)
{
    debug("Working stack (%u items):\n", sam_stack->sp - sam_program_len);
    print_stack(NULL, 0, s, sam_program_len, sam_stack->sp);
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
