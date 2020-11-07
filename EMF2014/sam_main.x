// -*- c -*-

#include <stdio.h>

#include "sam.h"
#include "sam_opcodes.h"

#define SAM_STACK_WORDS 4096
static sam_word_t sam_stack[SAM_STACK_WORDS] = {
#include "sam_program.h"
};

void main(void) {
  int ok = sam_init(&sam_stack[0], SAM_STACK_WORDS);
  size_t i;
  for (i = 0; i < 8; i++)
    fprintf(stderr, "stack word %zu: %x\n", i, sam_stack[i]);
  sam_word_t res = sam_run();
  sam_float_t real_res = *(sam_float_t *)&res;
  printf("%f\n", real_res);
}
