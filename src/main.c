#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "context.c"
#include "util.c"

const int ram_size = 640000;

int main(int argc, char* argv[]) {
  set_endianess();
  if (argc != 2) {
    printf("[Error] Please supply a file name!\n");
    return 0;
  }
  Context* context = context_from_file(ram_size, argv[1]);
  while (exec_instruction(context)) {
    printf("------------\n");
    printf("Register 1: %li\n", context->registers[1]);
    printf("Register 2: %li\n", context->registers[2]);
    printf("Register 5: %li\n", context->registers[5]);
    printf("------------\n");
  }

  return 0;
}