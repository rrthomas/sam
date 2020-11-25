// -*- c -*-

#include <assert.h>
#include <stdio.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_traps.h"
#include "sam_private.h"

int main(void) {
    sam_word_t error = SAM_ERROR_OK;
    HALT_IF_ERROR(sam_traps_init());
    int res = sam_init(&sam_stack[0], SAM_STACK_WORDS, sam_program_len);
    HALT_IF_ERROR(res);
    sam_print_stack();

 error:
#ifdef SAM_DEBUG
    debug("sam_run returns %d\n", sam_run());
    if (sam_traps_window_used()) {
        sam_dump_screen();
        getchar();
    }
#endif
    sam_traps_finish();
    return error;
}
