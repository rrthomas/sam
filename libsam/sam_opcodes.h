/* Sam instruction set description.

   (c) Reuben Thomas 1994-2026

   The package is distributed under the GNU Public License version 3, or,
   at your option, any later version.

   THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
   RISK.  */

#include "sam.h"

extern const sam_word_t SAM_FLOAT_TAG;
extern const sam_word_t SAM_FLOAT_TAG_MASK;
extern const int SAM_FLOAT_SHIFT;

extern const sam_word_t SAM_BLOB_TAG;
extern const sam_word_t SAM_BLOB_TAG_MASK;
extern const int SAM_BLOB_SHIFT;

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
extern const int SAM_ONE_INST_SHIFT;

extern const sam_word_t SAM_TRAP_BASE_MASK;

// Every word in a stack is tagged.

// Atom types (4 bits)
enum SAM_ATOM_TYPE {
  SAM_ATOM_NULL,
};

// Blob types (5 or 10 bits)
enum SAM_BLOB_TYPE {
  SAM_BLOB_RAW,
  SAM_BLOB_STACK,
  SAM_BLOB_MAP,
  SAM_BLOB_ITER,

  SAM_BLOB_TYPES,
};

// Instructions (5 bits)
enum SAM_INST {
  INST_NOP,
  INST_NEW,
  INST_S0,
  INST_DROP,
  INST_GET,
  INST_SET,
  INST_EXTRACT,
  INST_INSERT,
  INST_POP,
  INST_SHIFT,
  INST_APPEND,
  INST_PREPEND,
  INST_GO,
  INST_DO,
  INST_CALL,
  INST_IF,
  INST_WHILE,
  INST_NOT,
  INST_AND,
  INST_OR,
  INST_XOR,
  INST_EQ,
  INST_LT,
  INST_NEG,
  INST_ADD,
  INST_MUL,
  INST_0,
  INST_1,
  INST_MINUS_1,
  INST_2,
  INST_MINUS_2,
  INST_HALT,
};
_Static_assert(INST_HALT == 31);

// Useful aliases
#define INST_TRUE INST_MINUS_1
#define INST_FALSE INST_0
#define SAM_VALUE_FALSE SAM_INT_TAG
#define SAM_VALUE_NULL ((SAM_ATOM_NULL << SAM_ATOM_TYPE_SHIFT) | SAM_ATOM_TAG)
