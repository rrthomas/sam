/* Sam instruction set description.

   (c) Reuben Thomas 1994-2020

   The package is distributed under the GNU Public License version 3, or,
   at your option, any later version.

   THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
   RISK.  */


/* Instructions occupy a word, with the opcode in bits 0-7; the rest of word
   is an immediate operand. The opcodes are SAM_INSN_* below.
*/

/* Stack layout:

   Each stack item is either a VM instruction, or a stack.

   Stacks are stored as: BRA instruction (operand is number of words to
   KET), outer S0, stack items, KET (operand is negative number of words to
   BRA).
 */

#define SAM_OP_SHIFT 8
#define SAM_OP_MASK  ((1 << SAM_OP_SHIFT) - 1)

/* Opcodes.  */
enum {
  SAM_INSN_NOP = 0,
  SAM_INSN_INT,
  SAM_INSN_FLOAT,
  SAM_INSN__FLOAT,
  SAM_INSN_I2F,
  SAM_INSN_F2I,
  SAM_INSN_PUSH,
  SAM_INSN__PUSH,
  SAM_INSN_POP,
  SAM_INSN_DUP,
  SAM_INSN_SWAP,
  SAM_INSN_IDUP,
  SAM_INSN_ISET,
  SAM_INSN_BRA,
  SAM_INSN_KET,
  SAM_INSN_LINK,
  SAM_INSN_DO,
  SAM_INSN_IF,
  SAM_INSN_WHILE,
  SAM_INSN_LOOP,
  SAM_INSN_NOT,
  SAM_INSN_AND,
  SAM_INSN_OR,
  SAM_INSN_XOR,
  SAM_INSN_LSH,
  SAM_INSN_RSH,
  SAM_INSN_ARSH,
  SAM_INSN_EQ,
  SAM_INSN_LT,
  SAM_INSN_NEG,
  SAM_INSN_ADD,
  SAM_INSN_MUL,
  SAM_INSN_DIV,
  SAM_INSN_REM,
  SAM_INSN_POW,
  SAM_INSN_SIN,
  SAM_INSN_COS,
  SAM_INSN_DEG,
  SAM_INSN_RAD,
  SAM_INSN_HALT = 0xfe,
  SAM_INSN_TRAP = 0xff,
};
