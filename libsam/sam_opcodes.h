/* Sam instruction set description.

   (c) Reuben Thomas 1994-2025

   The package is distributed under the GNU Public License version 3, or,
   at your option, any later version.

   THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
   RISK.  */

#include "sam.h"

extern const int SAM_TAG_SHIFT;
extern const sam_word_t SAM_TAG_MASK;
extern const int SAM_ATOM_TYPE_SHIFT;
extern const int SAM_ATOM_TYPE_MASK;
extern const int SAM_BIATOM_TAG_SHIFT;
extern const sam_word_t SAM_BIATOM_TAG_MASK;
extern const int SAM_BIATOM_TYPE_SHIFT;
extern const sam_word_t SAM_BIATOM_TYPE_MASK;
extern const int SAM_ARRAY_TYPE_SHIFT;
extern const sam_word_t SAM_ARRAY_TYPE_MASK;
extern const int SAM_OPERAND_SHIFT;
extern const sam_word_t SAM_OPERAND_MASK;
extern const int SAM_LINK_SHIFT;
extern const sam_word_t SAM_LINK_MASK;

// Every word in a stack is tagged.

// Tags (bits 0-1)
enum SAM_TAG {
  SAM_TAG_LINK = 0,
  SAM_TAG_ATOM,
  SAM_TAG_BIATOM,
  SAM_TAG_ARRAY, // Used for BRA and KET.
};

// Atom types (bits 2-3)
enum SAM_ATOM_TYPE {
  SAM_ATOM_INST,
  SAM_ATOM_INT,
  SAM_ATOM_CHAR,
};

// Biatom tags (bit 2)
enum SAM_BIATOM_TAG {
  SAM_BIATOM_FIRST,
  SAM_BIATOM_SECOND,
};

// Biatom types (bits 3-7)
enum SAM_BIATOM_TYPE {
  SAM_BIATOM_WORD,
  SAM_BIATOM_FLOAT,
};

// Arrays are stored as: ARRAY instruction (operand is positive number of
// words to final ARRAY), a number of words, ARRAY (operand is negative
// number of words to initial ARRAY).

// Array types (bits 2-7)
enum SAM_ARRAY_TYPE {
  // Each stack item is either a VM instruction, or a stack.
  SAM_ARRAY_STACK,
  SAM_ARRAY_RAW,
};

// Instructions (bits 4-31)
enum SAM_INST {
  // Basic instructions.
  INST_NOP,
  INST_I2F,
  INST_F2I,
  INST_POP,
  INST_GET,
  INST_SET,
  INST_IGET,
  INST_ISET,
  INST_DO,
  INST_IF,
  INST_WHILE,
  INST_LOOP,
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
  INST_POW,
  INST_SIN,
  INST_COS,
  INST_DEG,
  INST_RAD,
  INST_HALT,

  // Graphics
  // Origin is 0,0 at top left.
  INST_BLACK,
  INST_WHITE,
  INST_DISPLAY_WIDTH,
  INST_DISPLAY_HEIGHT,
  INST_CLEARSCREEN,
  INST_SETDOT,
  INST_DRAWLINE,
  INST_DRAWRECT,
  INST_DRAWROUNDRECT,
  INST_FILLRECT,
  INST_DRAWCIRCLE,
  INST_FILLCIRCLE,
  INST_DRAWBITMAP,

  // TODO: Sound

  // TDOO: Input: joystick, buttons
};
