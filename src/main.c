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
  for (int i = 0; i < 30; i++) {
    exec_instruction(context);
    context_dump(context);
  }

  return 0;
}