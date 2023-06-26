#include <errno.h>
#include <stdint.h>
#include <stdio.h>

#include "vector.h"

#define DEBUG 1
#define eprintf(...) \
  if (DEBUG) fprintf(stderr, __VA_ARGS__);
#define try(expr) \
  if (expr == -1) printf("[Error] errno: %i\n", errno), exit(0);

typedef uint_least8_t byte;
typedef uint64_t u64;

static int is_big_endian = 2;

void set_endianess() {
  eprintf("[LOG] Setting endianess!\n");
  byte bytes[4] = {0x80, 0, 0, 0};
  uint32_t *num = (uint32_t *)&bytes;
  if (*num == 0x7fffffff) {
    is_big_endian = 1;
    eprintf("[LOG] System is big endian!\n");
  } else {
    is_big_endian = 0;
    eprintf("[LOG] System is little endian!\n");
  }
}
void invert_bytes(void *thang, int byte_count) {
  byte *thing = thang;
  byte tmp = 0;
  for (int i = 0; i < (byte_count / 2); i++) {
    tmp = thing[i];
    thing[i] = thing[byte_count - 1 - i];
    thing[byte_count - 1 - i] = tmp;
  }
}
u64 from_be_bytes(byte *bytes) {
  u64 out = *(u64 *)bytes;
  if (!is_big_endian) {
    invert_bytes(&out, 8);
  }
  return out;
}
Vector from_file(char *path) {
  eprintf("[LOG] Opening file: %s\n", path);
  FILE *bytecode_stream = fopen(path, "r");
  if (bytecode_stream == NULL) {
    switch (errno) {
      case 2:
        printf("[Error] Supplied file does not exist!\n");
        exit(0);
      default:
        printf("[Error] Couldn't open file!\n[Error] errno: %i\n", errno);
        exit(0);
    }
  }
  try(fseek(bytecode_stream, 0, SEEK_END));
  long bytecode_size = ftell(bytecode_stream);
  try(bytecode_size);
  eprintf("[LOG] File size: %li\n", bytecode_size);
  try(fseek(bytecode_stream, 0, SEEK_SET));
  Vector bytecode = vector.make(bytecode_size);
  for (int c = fgetc(bytecode_stream); c != EOF; c = fgetc(bytecode_stream)) {
    vector.push(&bytecode, (byte)c);
  }
  return bytecode;
}