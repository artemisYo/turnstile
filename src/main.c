#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

static int is_big_endian = 2;

void set_endianess() {
  uint8_t bytes[4] = {0x80, 0, 0, 0}; 
  uint32_t *num = (uint32_t*) &bytes;
  if (*num == 0x80000000) {
    is_big_endian = 1;
  } else {
    is_big_endian = 0;
  }
}
void invert_bytes(void* thang, int byte_count) {
  uint8_t *thing = thang;
  uint8_t tmp = 0;
  for (int i = 0; i < (byte_count / 2); i++) {
    tmp = thing[i];
    thing[i] = thing[byte_count - 1 - i];
    thing[byte_count - 1 - i] = tmp;
  }
}


int main() {
  char c[3] = {'a', 'b', 'c'};
  printf("%c%c%c\n", c[0], c[1], c[2]);
  invert_bytes(&c, 3);
  printf("%c%c%c\n", c[0], c[1], c[2]);
}