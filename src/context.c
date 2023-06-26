#include <stdint.h>

#include "util.c"
#include "vector.h"

typedef uint_least32_t instruction;
typedef uint_least8_t byte;
typedef uint64_t u64;
typedef uint16_t u16;
typedef uint8_t u8;

typedef struct {
  byte *base, *entry, *code, *data, *vars;
  u64 code_size, data_size, vars_size;
} Prelude;

typedef struct {
  u64 registers[32];
  Prelude prelude;
  Vector bytecode;
  byte *ip;
  u8 cmp_flags;
} Context;

Context context_new(Vector bytecode, Prelude prelude) {
  Context out = {{0}, prelude, bytecode, prelude.code, 0};
  return out;
}

Prelude read_prelude(byte *bytecode) {
  eprintf("[LOG] Reading prelude!\n");
  Prelude out = {0};
  out.base = bytecode;
  u64 entry = from_be_bytes(bytecode);
  eprintf("[LOG] Point of entry read as: %016lX\n", entry);
  byte *meta = bytecode + 8;
  for (; meta[0] != 0; meta += 17) {
    byte **tmp_loc = NULL;
    u64 *tmp_sz = 0;
    switch (meta[0]) {
      case 0xa0:

        eprintf("[LOG] Read code id!\n");
        tmp_loc = &out.code;
        tmp_sz = &out.code_size;
        break;
      case 0xa1:
        eprintf("[LOG] Read data id!\n");
        tmp_loc = &out.data;
        tmp_sz = &out.data_size;
        break;
      case 0xa2:
        eprintf("[LOG] Read vars id!\n");
        tmp_loc = &out.vars;
        tmp_sz = &out.vars_size;
        break;
      default:
        printf("[Error] Meta contains undefined id!\n[Error] found: %X\n",
               meta[0]);
        exit(0);
    }
    u64 loc = from_be_bytes(meta + 1);
    eprintf("[LOG] Read location as: %016X\n", loc);
    *tmp_loc = bytecode + loc;
    *tmp_sz = from_be_bytes(meta + 9);
    eprintf("[LOG] Read size as:     %016X\n", *tmp_sz);
  }
  return out;
}

void exec_instruction(Context *context) {
  eprintf("[LOG] Executing instruction!\n");
  instruction inst = *((instruction *)context->ip);
  eprintf("[LOG] Instruction read as: %08X\n", inst);
  byte destination, source, functional, opcode;
  u16 immediate;
  destination = inst >> 27;
  eprintf("[LOG] Destination read as: %i\n", destination);
  source = (inst >> 22) & 0b11111;
  eprintf("[LOG] Source read as:      %i\n", source);
  immediate = (inst >> 10) & 0b111111111111;
  eprintf("[LOG] Immediate read as:   %i\n", immediate);
  functional = (inst >> 7) & 0b111;
  eprintf("[LOG] Functional read as:  %i\n", functional);
  opcode = inst & 0b1111111;
  eprintf("[LOG] Opcode read as:      %i\n", opcode);
}