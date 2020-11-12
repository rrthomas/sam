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
  assert(sam_traps_init() == SAM_ERROR_OK);
  sam_word_t error = SAM_ERROR_OK;
  int ok = sam_init(&sam_stack[0], SAM_STACK_WORDS,
#include "sam_program_len.h"
           ) == SAM_ERROR_OK;
  assert(ok);
  printf("sam_run returns %d\n", sam_run());

  sam_print_stack();

 error:
  getchar();
  sam_traps_finish();
  return error;
}
