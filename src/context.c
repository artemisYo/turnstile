#include <stdint.h>

#include "util.c"
#include "vector.h"

typedef uint_least8_t byte;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef enum {
  // None/R -- B
  Halt = 0b1000000000000000,
  NoOp = 0b1000000000000001,
  Inc = 0b1000000000000010,
  Dec = 0b1000000000000011,
  // R, r -- D
  Add = 0b0000000000000000,
  Sub = 0b0000000000000001,
  Dupe = 0b0000000000000010,
  // R, r, s, i -- Qf
  Jump = 0b0100000000000000,
  JIfE = 0b0100000000000001,
  JIfG = 0b0100000000000010,
  JIfL = 0b0100000000000011,
  JIGE = 0b0100000000000100,
  JILE = 0b0100000000000101,
  JINE = 0b0100000000000110,
  // R, i -- Qi
  MvSg = 0b0010000000000000,
  MvDb = 0b0010000000000001,
  MvQd = 0b0010000000000010,
  MvFl = 0b0010000000000011,
  // R, r, i -- Qo
  LdSg = 0b0010000001000000,
  LdDb = 0b0010000001000001,
  LdQd = 0b0010000001000010,
  LdFl = 0b0010000001000011,
  StSg = 0b0010000001000100,
  StDb = 0b0010000001000101,
  StQd = 0b0010000001000110,
  StFl = 0b0010000001000111,
} FuncCode;

typedef struct {
  u64 registers[32];
  Prelude prelude;
  Vector bytecode;
  byte *ip;
} Context;

Context context_new(Vector bytecode, Prelude prelude) {
  return (Context){.registers = {0},
                   .prelude = prelude,
                   .bytecode = bytecode,
                   .ip = prelude.entry};
}

typedef struct {
  u64 *destination, *source, *alt_source;
  FuncCode functional;
  u16 immediate;
} Instruction;

Instruction decode_instruction(Context *context) {
  u8 mark = ((*(u8 *)context->ip) >> 5);
  if (mark & 0b100) {
    return decode_instruction_type_b(context);
  } else if (mark == 0b000) {
    return decode_instruction_type_d(context);
  } else if (mark == 0b010) {
    return decode_instruction_type_qf(context);
  } else if ((*(u16 *)context->ip) & 0b10000000) {
    return decode_instruction_type_qo(context);
  }
  return decode_instruction_type_qi(context);
}

static inline u8 get_dest(byte *ip) { return (*(u8 *)ip) & 0b11111; }
static inline u8 get_src(byte *ip) { return (*(u8 *)ip + 1) >> 3; }
static inline u16 get_immediate(byte *ip, u8 bitcount) {
  return from_be_bytes_16(ip + 2) & ((1 << bitcount) - 1);
}

Instruction decode_instruction_type_b(Context *context) {
  const u16 mask = 0b1000000000000000;
  u8 raw = *(u8 *)context->ip;
  u16 func = raw >> 5;
  func &= 0b11;
  func |= mask;
  u8 dest = get_dest(context->ip);
  return (Instruction){.functional = func,
                       .destination = &context->registers[dest],
                       .alt_source = {0},
                       .immediate = {0},
                       .source = {0}};
}
Instruction decode_instruction_type_d(Context *context) {
  const u16 mask = 0b0000000000000000;
  u16 raw = from_be_bytes_16(context->ip);
  u16 func = raw;
  func &= 0b111;
  func |= mask;
  u8 dest = get_dest(context->ip);
  u8 src = get_src(context->ip);
  return (Instruction){
      .functional = func,
      .destination = &context->registers[dest],
      .source = &context->registers[src],
      .alt_source = {0},
      .immediate = {0},
  };
}
Instruction decode_instruction_type_qi(Context *context) {
  const u16 mask = 0b0010000000000000;
  u32 raw = from_be_bytes_32(context->ip);
  u16 func = raw >> 16;
  func &= 0xff;
  func |= mask;
  u8 dest = get_dest(context->ip);
  u16 immd = get_immediate(context->ip, 16);
  return (Instruction){
      .functional = func,
      .destination = &context->registers[dest],
      .immediate = immd,
      .source = {0},
      .alt_source = {0},
  };
}
Instruction decode_instruction_type_qo(Context *context) {
  const u16 mask = 0b0010000001000000;
  u32 raw = from_be_bytes_32(context->ip);
  u16 func = raw >> 12;
  func &= 0b01111111;
  func |= mask;
  u8 dest = get_dest(context->ip);
  u8 src = get_src(context->ip);
  u16 immd = get_immediate(context->ip, 12);
  return (Instruction){
      .functional = func,
      .destination = &context->registers[dest],
      .source = &context->registers[src],
      .immediate = immd,
      .alt_source = {0},
  };
}
Instruction decode_instruction_type_qf(Context *context) {
  const u16 mask = 0b0100000000000000;
  u32 raw = from_be_bytes_32(context->ip);
  u16 func = raw >> 13;
  func &= 0b00111111;
  func |= mask;
  u8 dest = get_dest(context->ip);
  u8 src = get_src(context->ip);
  u16 immd = get_immediate(context->ip, 8);
  u8 alt_src = raw >> 8;
  alt_src &= 0b11111;
  return (Instruction){.functional = func,
                       .destination = &context->registers[dest],
                       .source = &context->registers[src],
                       .immediate = immd,
                       .alt_source = &context->registers[alt_src]};
}

typedef struct {
  byte *base, *entry, *code, *data, *vars;
  u64 code_size, data_size, vars_size;
} Prelude;

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
