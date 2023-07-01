#ifndef UTIL_DEFINED
#define UTIL_DEFINED

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG 1
#define eprintf(...) \
  if (DEBUG) fprintf(stderr, __VA_ARGS__);
#define try(expr) \
  if (expr == -1) printf("[Error] errno: %i\n", errno), exit(0);

typedef uint_least8_t byte;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;

static int is_big_endian = 2;

void set_endianess() {
  eprintf("[LOG] Setting endianess!\n");
  if (*(uint32_t *)&(byte[]){0x80, 0, 0, 0} == 0x7fffffff) {
    is_big_endian = 1;
    eprintf("[LOG] System is big endian!\n");
  } else {
    is_big_endian = 0;
    eprintf("[LOG] System is little endian!\n");
  }
}
void invert_bytes(void *thang, int byte_count) {
  byte *thing = thang;
  for (int i = 0; i < (byte_count / 2); i++) {
    byte tmp = thing[i];
    thing[i] = thing[byte_count - 1 - i];
    thing[byte_count - 1 - i] = tmp;
  }
}
void write_be_bytes(byte *bytes, int count, byte buffer[count]) {
  for (int i = 0; i < count; i++) {
    int idx = i;
    if (!is_big_endian) idx = count - 1 - i;
    buffer[idx] = bytes[i];
  }
}
u64 from_be_bytes(byte *bytes) {
  u64 out = *(u64 *)bytes;
  if (!is_big_endian) {
    invert_bytes(&out, 8);
  }
  return out;
}
u32 from_be_bytes_32(byte *bytes) {
  u32 out = *(u32 *)bytes;
  if (!is_big_endian) {
    invert_bytes(&out, 4);
  }
  return out;
}
u16 from_be_bytes_16(byte *bytes) {
  u32 out = *(u16 *)bytes;
  if (!is_big_endian) {
    invert_bytes(&out, 2);
  }
  return out;
}
int file_into(int buffer_size, byte buffer[buffer_size], char *path) {
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
  if (bytecode_size > buffer_size)
    printf("[Error] Allocated RAM-size is smaller than the bytecode!\n"),
        exit(0);
  eprintf("[LOG] File size: %li\n", bytecode_size);
  try(fseek(bytecode_stream, 0, SEEK_SET));
  for (int c = fgetc(bytecode_stream), i = 0; c != EOF && i < buffer_size;
       c = fgetc(bytecode_stream), i++) {
    buffer[i] = c;
  }
  return bytecode_size;
}
#endif