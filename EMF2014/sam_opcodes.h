/* Sam instruction set description.

   (c) Reuben Thomas 1994-2020

   The package is distributed under the GNU Public License version 3, or,
   at your option, any later version.

   THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
   RISK.  */


/* Instructions occupy a word, and take the following forms:

   32-bit Sam:

   1. Opcode in bits 0-1, rest of word is immediate operand.
      The opcodes are SAM_OP_* below masked with SAM_OP1_MASK.
   2. Bits 0-1 are 11, opcode in bits 2-3, rest of word is immediate operand.
      The opcodes are SAM_OP_* below masked with SAM_OP2_MASK.
   3. Bits 0-3 are 1111, rest of word is instruction opcode.
      The opcodes are SAM_INSN_* below.

   64-bit Sam:

   1. Opcode in bits 0-2, rest of word is immediate operand.
      The opcodes are SAM_OP_* below masked with SAM_OP1_MASK == SAM_OP2_MASK.
   2. Bits 0-3 are 111, rest of word is instruction opcode.
      The opcodes are SAM_INSN_* below.
*/


/* Sam opcode info.  */
typedef struct sam_opc_info_t
{
  int opcode;		/* opcode bits for particular instruction.  */
  const char * name;	/* instruction name, where applicable.  */
} sam_opc_info_t;

/* Instruction types.  */
#define SAM_OP_SHIFT 2
enum {
  SAM_OP_MASK    = 0x3,

  /* Bits 0-1.  */
  SAM_OP_STACK    = 0x0,
  SAM_OP_LIT      = 0x1,
  SAM_OP_TRAP     = 0x2,
  SAM_OP_INSN     = 0x3,
};

/* Largest trap code.  */
#if SAM_WORD_BYTES == 4
#define SAM_MAX_TRAP ((1 << 30) - 1)
#else
#define SAM_MAX_TRAP ((1 << 62) - 1)
#endif

/* OP_INSN opcodes.  */
enum {
  SAM_INSN_NOP = 0,
  SAM_INSN_NOT,
  SAM_INSN_AND,
  SAM_INSN_OR,
  SAM_INSN_XOR,
  SAM_INSN_LSH,
  SAM_INSN_RSH,
  SAM_INSN_ARSH,
  SAM_INSN_POP,
  SAM_INSN_DUP,
  SAM_INSN_SET,
  SAM_INSN_SWAP,
  SAM_INSN_IDUP,
  SAM_INSN_ISET,
  SAM_INSN_PUSH,
  SAM_INSN_BRA,
  SAM_INSN_KET,
  SAM_INSN_DO,
  SAM_INSN_S,
  SAM_INSN_K,
  SAM_INSN_NEG,
  SAM_INSN_ADD,
  SAM_INSN_MUL,
  SAM_INSN_DIV,
  SAM_INSN_EQ,
  SAM_INSN_LT,
  SAM_INSN_THROW,
  SAM_INSN_CATCH,

  SAM_INSN_UNDEFINED = 0x3f
};
