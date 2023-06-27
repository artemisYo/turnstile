#include <stdint.h>

#include "util.c"
#include "vector.h"

typedef uint_least32_t instruction;
typedef uint_least8_t byte;
typedef uint64_t u64;
typedef uint16_t u16;
typedef uint8_t u8;

typedef enum {
  Halt = 0b0000000000,
  NoOp = 0b0010000000,
  Inc  = 0b0000010000,
  Dec  = 0b0010010000,
  Add  = 0b0000010001,
  Sub  = 0b0010010001,
  Jmp  = 0b0000010010,
  JIfE = 0b0010010010,
  JIfG = 0b0100010010,
  JIfL = 0b0110010010,
  JIGE = 0b1000010010,
  JILE = 0b1010010010,
  JINE = 0b1100010010,
  Cmp  = 0b1110010010,
  Mov  = 0b0000010011,
  StrN = 0b0000010100,
  StrO = 0b0010010100,
  StrD = 0b0100010100,
  StrT = 0b0110010100,
  StrQ = 0b1000010100,
  StrF = 0b1010010100,
} OpCode;

typedef struct {
  OpCode operation;
  byte destination, source;
  u16 immediate;
} DecodedInstruction;

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
  for (byte *meta = bytecode + 8; meta[0] != 0; meta += 17) {
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
    eprintf("[LOG] Read location as: %016lX\n", loc);
    *tmp_loc = bytecode + loc;
    *tmp_sz = from_be_bytes(meta + 9);
    eprintf("[LOG] Read size as:     %016lX\n", *tmp_sz);
  }
  return out;
}

DecodedInstruction decode_instruction(Context *context) {
  eprintf("[LOG] Decoding instruction!\n");
  instruction inst = *((instruction *)context->ip);
  eprintf("[LOG] Instruction read as: %08X\n", inst);
  DecodedInstruction out = {0};
  out.destination = inst >> 27;
  eprintf("[LOG] Destination read as: %i\n", out.destination);
  out.source = (inst >> 22) & 0b11111;
  eprintf("[LOG] Source read as:      %i\n", out.source);
  out.immediate = (inst >> 10) & 0b111111111111;
  eprintf("[LOG] Immediate read as:   %i\n", out.immediate);
  out.functional = (inst >> 7) & 0b111;
  eprintf("[LOG] Functional read as:  %i\n", out.functional);
  out.opcode = inst & 0b1111111;
  eprintf("[LOG] Opcode read as:      %i\n", out.opcode);
  return out;
}

void exec_instruction(Context *context) {
  eprintf("[LOG] Executing instruction!\n");
  DecodedInstruction inst = decode_instruction(context);
}