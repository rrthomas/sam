// -*- c -*-

#include <assert.h>
#include <stdio.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_private.h"

#define SAM_STACK_WORDS 4096
static sam_word_t sam_stack[SAM_STACK_WORDS] = {
#include "sam_program.h"
};

int main(void) {
  int ok = sam_init(&sam_stack[0], SAM_STACK_WORDS,
#include "sam_program_len.h"
           ) == SAM_ERROR_OK;
  assert(ok);
  printf("sam_run returns %d\n", sam_run());

  sam_print_stack();

  return 0;
}
