#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "context.c"
#include "util.c"

int main(int argc, char *argv[]) {
  set_endianess();
  if (argc != 2) {
    printf("[Error] Please supply a file name!\n");
    return 0;
  }
  Vector bytecode = from_file(argv[1]);
  vector.dump(bytecode);
  Prelude prelude = read_prelude(bytecode.buffer);
  Context context = context_new(bytecode, prelude);
  vector.free(context.bytecode);
  return 0;
}