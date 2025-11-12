/* Sam instruction set description.

   (c) Reuben Thomas 1994-2025

   The package is distributed under the GNU Public License version 3, or,
   at your option, any later version.

   THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
   RISK.  */

// Instructions occupy a word, with the opcode in bits 0-3; the rest of word
// is an immediate operand. The opcodes are SAM_INSN_* below.

// Stack layout:
//
// Each stack item is either a VM instruction, or a stack.
//
// Stacks are stored as: BRA instruction (operand is number of words to
// KET), a number of words, KET (operand is negative number of words to
// BRA).

#include "sam.h"

extern const int SAM_OP_SHIFT;
extern const sam_word_t SAM_OP_MASK;

// Opcodes
enum SAM_INSN {
  SAM_INSN_TRAP = 0,
  SAM_INSN_INT,
  SAM_INSN_FLOAT,
  SAM_INSN__FLOAT,
  SAM_INSN_PUSH,
  SAM_INSN__PUSH,
  SAM_INSN_BRA,
  SAM_INSN_KET,
  SAM_INSN_LINK,
};

// Traps
enum SAM_TRAP {
  // Basic instructions.
  TRAP_NOP,
  TRAP_I2F,
  TRAP_F2I,
  TRAP_POP,
  TRAP_GET,
  TRAP_SET,
  TRAP_IGET,
  TRAP_ISET,
  TRAP_DO,
  TRAP_IF,
  TRAP_WHILE,
  TRAP_LOOP,
  TRAP_NOT,
  TRAP_AND,
  TRAP_OR,
  TRAP_XOR,
  TRAP_LSH,
  TRAP_RSH,
  TRAP_ARSH,
  TRAP_EQ,
  TRAP_LT,
  TRAP_NEG,
  TRAP_ADD,
  TRAP_MUL,
  TRAP_DIV,
  TRAP_REM,
  TRAP_POW,
  TRAP_SIN,
  TRAP_COS,
  TRAP_DEG,
  TRAP_RAD,
  TRAP_HALT,

  // Graphics
  // Origin is 0,0 at top left.
  TRAP_BLACK,
  TRAP_WHITE,
  TRAP_DISPLAY_WIDTH,
  TRAP_DISPLAY_HEIGHT,
  TRAP_CLEARSCREEN,
  TRAP_SETDOT,
  TRAP_DRAWLINE,
  TRAP_DRAWRECT,
  TRAP_DRAWROUNDRECT,
  TRAP_FILLRECT,
  TRAP_DRAWCIRCLE,
  TRAP_FILLCIRCLE,
  TRAP_DRAWBITMAP,

  // TODO: Sound

  // TDOO: Input: joystick, buttons
};
