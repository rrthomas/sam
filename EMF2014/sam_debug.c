#include <stdio.h>

#include "sam.h"

void sam_print_stack(void)
{
  // Print the stack.
  fprintf(stderr, "Stack: (%u words)\n", sam_sp);
  sam_uword_t i;
  for (i = 0; i < sam_sp; i++)
    fprintf(stderr, "%d 0x%x\n", sam_s0[i], sam_s0[i]);
}
