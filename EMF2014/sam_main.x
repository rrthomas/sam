// -*- c -*-

#include <assert.h>
#include <stdio.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_traps.h"
#include "sam_private.h"

#define SAM_STACK_WORDS 4096
static sam_word_t sam_stack[SAM_STACK_WORDS] = {
#include "sam_program.h"
};

int main(void) {
    sam_word_t error = SAM_ERROR_OK;
    HALT_IF_ERROR(sam_traps_init());
    int res = sam_init(&sam_stack[0], SAM_STACK_WORDS,
#include "sam_program_len.h"
                       );
    HALT_IF_ERROR(res);
    sam_print_stack();
    printf("sam_run returns %d\n", sam_run());

 error:
#ifdef SAM_DEBUG
    if (sam_traps_window_used())
        getchar();
#endif
    sam_traps_finish();
    return error;
}
