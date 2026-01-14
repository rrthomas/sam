/* Sam instruction set description.

   (c) Reuben Thomas 1994-2025

   The package is distributed under the GNU Public License version 3, or,
   at your option, any later version.

   THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
   RISK.  */

#include "sam.h"

extern const sam_word_t SAM_FLOAT_TAG;
extern const sam_word_t SAM_FLOAT_TAG_MASK;
extern const int SAM_FLOAT_SHIFT;

extern const sam_word_t SAM_STACK_TAG;
extern const sam_word_t SAM_STACK_TAG_MASK;
extern const int SAM_STACK_SHIFT;

extern const sam_word_t SAM_INT_TAG;
extern const sam_word_t SAM_INT_TAG_MASK;
extern const int SAM_INT_SHIFT;

extern const sam_word_t SAM_ATOM_TAG;
extern const sam_word_t SAM_ATOM_TAG_MASK;
extern const sam_word_t SAM_ATOM_TYPE_MASK;
extern const int SAM_ATOM_TYPE_SHIFT;
extern const int SAM_ATOM_SHIFT;

extern const sam_word_t SAM_TRAP_TAG;
extern const sam_word_t SAM_TRAP_TAG_MASK;
extern const int SAM_TRAP_FUNCTION_SHIFT;

extern const sam_word_t SAM_INSTS_TAG;
extern const sam_word_t SAM_INSTS_TAG_MASK;
extern const sam_word_t SAM_INST_SET_MASK;
extern const int SAM_INST_SET_SHIFT;
extern const int SAM_INSTS_SHIFT;
extern const sam_word_t SAM_INST_MASK;
extern const int SAM_INST_SHIFT;

extern const sam_word_t SAM_TRAP_BASE_MASK;

// Every word in a stack is tagged.

// Arrays are stored as: ARRAY instruction (operand is positive number of
// words to final ARRAY), a number of words, ARRAY (operand is negative
// number of words to initial ARRAY).

// Array types (5 or 10 bits)
enum SAM_ARRAY_TYPE {
  SAM_ARRAY_STACK,
  SAM_ARRAY_RAW,
};

// Instructions (5 bits)
enum SAM_INST {
  INST_NOP,
  INST_POP,
  INST_GET,
  INST_SET,
  INST_EXTRACT,
  INST_INSERT,
  INST_IGET,
  INST_ISET,
  INST_GO,
  INST_DO,
  INST_IF,
  INST_WHILE,
  INST_NOT,
  INST_AND,
  INST_OR,
  INST_XOR,
  INST_LSH,
  INST_RSH,
  INST_ARSH,
  INST_EQ,
  INST_LT,
  INST_NEG,
  INST_ADD,
  INST_MUL,
  INST_DIV,
  INST_REM,
  INST_0,
  INST_1,
  INST_MINUS_1,
  INST_2,
  INST_MINUS_2,
  INST_HALT,
};

// Useful aliases
#define INST_TRUE INST_MINUS_1
#define INST_FALSE INST_0
