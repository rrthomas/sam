// The interpreter main loop.
//
// (c) Reuben Thomas 1994-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#ifdef SAM_DEBUG
#include <stdio.h>
#include <stdlib.h>
#endif

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"

// Instruction constants
const int SAM_OP_SHIFT = 3;
const sam_word_t SAM_OP_MASK = (1 << SAM_OP_SHIFT) - 1;


// Execution function
sam_word_t sam_run(void)
{
    sam_word_t error = SAM_ERROR_OK;

    for (;;) {
        sam_uword_t ir;
        HALT_IF_ERROR(sam_stack_peek(sam_pc++, &ir));
        sam_word_t operand = ARSHIFT((sam_word_t)ir, SAM_OP_SHIFT);
#ifdef SAM_DEBUG
        debug("sam_run: pc0 = %u, pc = %u, sp = %u, ir = %x\n", sam_pc0, sam_pc - 1, sam_sp, ir);
        sam_print_working_stack();
#endif

        switch (ir & SAM_OP_MASK) {
        case SAM_INSN_TRAP:
            HALT_IF_ERROR(sam_trap((sam_uword_t)operand));
            break;
        case SAM_INSN_INT:
#ifdef SAM_DEBUG
            debug("int\n");
#endif
            PUSH_WORD(ir);
            break;
        case SAM_INSN_FLOAT:
#ifdef SAM_DEBUG
            debug("float\n");
#endif
            {
                sam_uword_t operand2;
                HALT_IF_ERROR(sam_stack_peek(sam_pc++, &operand2));
                if ((operand2 & SAM_OP_MASK) != SAM_INSN__FLOAT)
                    HALT(SAM_ERROR_UNPAIRED_FLOAT);
                PUSH_WORD(ir);
                PUSH_WORD(operand2);
            }
            break;
        case SAM_INSN__FLOAT:
#ifdef SAM_DEBUG
            debug("_float\n");
#endif
            HALT(SAM_ERROR_UNPAIRED_FLOAT);
            break;
        case SAM_INSN_PUSH:
#ifdef SAM_DEBUG
            debug("push\n");
#endif
            {
                sam_uword_t operand2;
                HALT_IF_ERROR(sam_stack_peek(sam_pc++, &operand2));
                if ((operand2 & SAM_OP_MASK) != SAM_INSN__PUSH)
                    HALT(SAM_ERROR_UNPAIRED_PUSH);
                PUSH_WORD((ir & ~SAM_OP_MASK) | ((operand2 >> SAM_OP_SHIFT) & SAM_OP_MASK));
            }
            break;
        case SAM_INSN__PUSH:
#ifdef SAM_DEBUG
            debug("_push\n");
#endif
            HALT(SAM_ERROR_UNPAIRED_PUSH);
            break;
        case SAM_INSN_STACK:
#ifdef SAM_DEBUG
            debug("%s\n", operand > 0 ? "bra" : "ket");
#endif
            if (operand > 0) {
                PUSH_LINK(sam_pc - 1);
                sam_pc += operand; // Skip to next instruction
            } else {
                RET;
            }
            break;
        case SAM_INSN_LINK:
#ifdef SAM_DEBUG
            debug("link\n");
#endif
            PUSH_WORD(ir); // Push the same link on the stack
            break;
        default:
#ifdef SAM_DEBUG
            debug("ERROR_INVALID_OPCODE\n");
#endif
            HALT(SAM_ERROR_INVALID_OPCODE);
            break;
        }

        sam_traps_process_events();
    }

error:
    DO(sam_pc); // Save pc, pc0 so we can restart.
    return error;
}
