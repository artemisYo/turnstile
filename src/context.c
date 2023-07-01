#ifndef CONTEXT_DEFINED
#define CONTEXT_DEFINED
#include <stdint.h>

#include "interrupts.c"
#include "util.c"

#define todo() \
  printf("[%04i] This part of the program is not yet implemented!\n", __LINE__);

typedef uint_least8_t byte;
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define name_of(aaa) \
  { #aaa, aaa }

typedef enum {
  // None -- Be
  Halt = 0b1000000000000000,
  NoOp = 0b1000000000000001,
  DIHT = 0b1000000000000010,
  SIHT = 0b1000000000000011,
  OIHT = 0b1000000000000100,
  Retn = 0b1000000000000101,
  // R -- Br
  Itrt = 0b1100000000000000,
  ItRt = 0b1100000000000001,
  // R, r -- D
  Dupe = 0b0000000000000010,
  And = 0b0000000000000011,
  Or = 0b0000000000000100,
  Xor = 0b0000000000000101,
  Not = 0b0000000000000110,
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
  LIHT = 0b0010000000000100,
  Call = 0b0010000000000101,
  // R, r, i -- Qo
  LdSg = 0b0010000001000000,
  LdDb = 0b0010000001000001,
  LdQd = 0b0010000001000010,
  LdFl = 0b0010000001000011,
  StSg = 0b0010000001000100,
  StDb = 0b0010000001000101,
  StQd = 0b0010000001000110,
  StFl = 0b0010000001000111,
  Add = 0b0010000001001000,
  Sub = 0b0010000001001001,
  // Any - V
  Put = 0b1010000000000000,
  // R, i -- Dpi
} FuncCode;

char *get_funccode_name(FuncCode code) {
  const struct {
    char *name;
    FuncCode code;
  } lookup[37] = {
      name_of(Halt), name_of(NoOp), name_of(DIHT), name_of(SIHT), name_of(OIHT),
      name_of(Retn), name_of(Itrt), name_of(ItRt), name_of(Dupe), name_of(And),
      name_of(Or),   name_of(Xor),  name_of(Not),  name_of(Jump), name_of(JIfE),
      name_of(JIfG), name_of(JIfL), name_of(JIGE), name_of(JILE), name_of(JINE),
      name_of(MvSg), name_of(MvDb), name_of(MvQd), name_of(MvFl), name_of(LIHT),
      name_of(Call), name_of(LdSg), name_of(LdDb), name_of(LdQd), name_of(LdFl),
      name_of(StSg), name_of(StDb), name_of(StQd), name_of(StFl), name_of(Add),
      name_of(Sub),  {"Undef", 0},
  };
  int i = 0;
  for (; i < 36; i++) {
    if (lookup[i].code == code) {
      break;
    }
  }
  return lookup[i].name;
}

typedef struct {
  byte *base, *entry, *code, *data, *vars;
  u64 code_size, data_size, vars_size, static_size;
} Prelude;

Prelude read_prelude(byte *bytecode) {
  eprintf("[LOG] Reading prelude!\n");
  Prelude out = {0};
  out.base = bytecode;
  out.static_size = from_be_bytes(bytecode);
  out.entry = bytecode + from_be_bytes(bytecode + 8);
  eprintf("[LOG] Point of entry read as: %016lX\n", out.entry - bytecode);
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
typedef struct {
  u8 privileges;
  u16 iht_size;
  u64 buffer_size;
  byte *ip, *iht, *buffer;
  Prelude prelude;
  u64 registers[32];
} Context;

Context *context_from_file(int buffer_size, char *path) {
  Context *out = malloc(sizeof(Context) + buffer_size);
  out->buffer = (byte *)out + sizeof(Context);
  out->privileges = 1;
  file_into(buffer_size, out->buffer, path);
  out->buffer_size = buffer_size;
  out->prelude = read_prelude(out->buffer);
  out->ip = out->prelude.entry;
  out->registers[31] = out->prelude.static_size;
  return out;
}

void context_dump(Context *context) {
  eprintf("ip: %016lX\n", (intptr_t)(context->ip - context->buffer));
  for (int i = 0; i < 32; i++)
    eprintf("r%02i: %016lX\n", i, context->registers[i]);
}

typedef struct {
  u64 *destination, *source, *alt_source, immediate;
  FuncCode functional;
} Instruction;

static inline u8 get_dest(byte *ip) { return *ip & 0b11111; }
static inline u8 get_src(byte *ip) {
  return (from_be_bytes_16(ip) >> 3) & 0b11111;
}
static inline u16 get_immediate(byte *ip, u8 bitcount) {
  return from_be_bytes_16(ip + 2) & ((1 << bitcount) - 1);
}

Instruction decode_instruction_type_be(Context *context) {
  const u16 mask = 0b1000000000000000;
  u8 raw = *(u8 *)context->ip;
  eprintf("[LOG] Read instruction as: %01X\n", raw);
  u16 func = raw;
  func &= 0b11111;
  func |= mask;
  u8 dest = get_dest(context->ip);
  return (Instruction){.functional = func,
                       .destination = &context->registers[dest],
                       .alt_source = 0,
                       .immediate = 0,
                       .source = 0};
}
Instruction decode_instruction_type_br(Context *context) {
  const u16 mask = 0b1100000000000000;
  u8 raw = *(u8 *)context->ip;
  eprintf("[LOG] Read instruction as: %01X\n", raw);
  u16 func = raw >> 5;
  func &= 0b1;
  func |= mask;
  u8 dest = get_dest(context->ip);
  return (Instruction){.functional = func,
                       .destination = &context->registers[dest],
                       .alt_source = 0,
                       .immediate = 0,
                       .source = 0};
}
Instruction decode_instruction_type_d(Context *context) {
  const u16 mask = 0b0000000000000000;
  u16 raw = from_be_bytes_16(context->ip);
  eprintf("[LOG] Read instruction as: %04X\n", raw);
  u16 func = raw;
  func &= 0b111;
  func |= mask;
  u8 dest = get_dest(context->ip);
  u8 src = get_src(context->ip);
  return (Instruction){
      .functional = func,
      .destination = &context->registers[dest],
      .source = &context->registers[src],
      .alt_source = 0,
      .immediate = 0,
  };
}
Instruction decode_instruction_type_qi(Context *context) {
  const u16 mask = 0b0010000000000000;
  u32 raw = from_be_bytes_32(context->ip);
  eprintf("[LOG] Read instruction as: %08X\n", raw);
  u16 func = raw >> 16;
  func &= 0xff;
  func |= mask;
  u8 dest = get_dest(context->ip);
  u16 immd = get_immediate(context->ip, 16);
  return (Instruction){
      .functional = func,
      .destination = &context->registers[dest],
      .immediate = immd,
      .source = 0,
      .alt_source = 0,
  };
}
Instruction decode_instruction_type_qo(Context *context) {
  const u16 mask = 0b0010000001000000;
  u32 raw = from_be_bytes_32(context->ip);
  eprintf("[LOG] Read instruction as: %08X\n", raw);
  u16 func = raw >> 12;
  func &= 0b111111;
  eprintf("[LOG] Qo func without mask: %i\n", func);
  func |= mask;
  u8 dest = get_dest(context->ip);
  u8 src = get_src(context->ip);
  u16 immd = get_immediate(context->ip, 12);
  return (Instruction){
      .functional = func,
      .destination = &context->registers[dest],
      .source = &context->registers[src],
      .immediate = immd,
      .alt_source = 0,
  };
}
Instruction decode_instruction_type_qf(Context *context) {
  const u16 mask = 0b0100000000000000;
  u32 raw = from_be_bytes_32(context->ip);
  eprintf("[LOG] Read instruction as: %08X\n", raw);
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
Instruction decode_instruction_type_dpi(Context *context) {
  const u16 mask = 0b0110000000000000;
  u16 raw = from_be_bytes_16(context->ip);
  u64 immd = from_be_bytes(context->ip + 2);
  eprintf("[LOG] Read instruction as: %04X%016lX\n", raw, immd);
  u16 func = raw;
  func &= 0x0f;
  func |= mask;
  u8 dest = get_dest(context->ip);
  return (Instruction){
      .functional = func,
      .destination = &context->registers[dest],
      .immediate = immd,
      .source = 0,
      .alt_source = 0,
  };
}
Instruction decode_instruction_type_v(Context *context) {
  const u16 mask = 0b1010000000000000;
  u8 raw = *context->ip;
  u16 func = raw;
  func &= 0b11111;
  func |= mask;
  return (Instruction){
      .functional = func,
      .destination = 0,
      .immediate = 0,
      .source = 0,
      .alt_source = 0,
  };
}
struct Cancer {
  Instruction inst;
  u8 sz;
} decode_instruction(Context *context) {
  eprintf("[LOG] Decoding instruction!\n");
  u8 mark = (from_be_bytes_16(context->ip) >> 13);
  u16 raw_mark = from_be_bytes_16(context->ip);
  eprintf("[LOG] Read mark as: %i\n", mark);
  eprintf("[LOG] Read instruction as type ");
  if (mark == 0b100) {
    eprintf("Be!\n");
    return (struct Cancer){decode_instruction_type_be(context), 1};
  } else if ((mark & 0b110) == 0b110) {
    eprintf("Br!\n");
    return (struct Cancer){decode_instruction_type_br(context), 1};
  } else if (mark == 0b000) {
    eprintf("D!\n");
    return (struct Cancer){decode_instruction_type_d(context), 2};
  } else if (mark == 0b010) {
    eprintf("Qf!\n");
    return (struct Cancer){decode_instruction_type_qf(context), 4};
  } else if (mark == 0b001 && raw_mark & 0b100) {
    eprintf("Qo!\n");
    return (struct Cancer){decode_instruction_type_qo(context), 4};
  } else if (mark == 0b001 && (raw_mark & 0b100) == 0) {
    eprintf("Qi!\n");
    return (struct Cancer){decode_instruction_type_qi(context), 4};
  } else if (mark == 0b011 && (raw_mark & 0b100) == 0) {
    eprintf("Dpi!\n");
    return (struct Cancer){decode_instruction_type_dpi(context), 10};
  } else if (mark == 0b101) {
    eprintf("V\n");
    return (struct Cancer){decode_instruction_type_v(context), 1};
  }
  printf("[Error] Invalid instruction type!\n");
  exit(0);
}
void invalid_access(char *message) {
  printf("%s", message);
  exit(0);
}
void jump_instruction(Context *context, Instruction instruction) {
  byte *address = (byte *)&context->buffer + *instruction.alt_source +
                  instruction.immediate;
  if (context->prelude.code > address ||
      address > context->prelude.code + context->prelude.code_size) {
    invalid_access("[Error] Jump outside of code segment!\n");
  }
  context->ip = address;
}
void store_instruction_check(Context *context, Instruction instruction) {
  byte *address = (byte *)context->buffer + *instruction.destination +
                  instruction.immediate;
  int padding;
  switch (instruction.functional) {
    case StSg:
      padding = 1;
      break;
    case StDb:
      padding = 2;
      break;
    case StQd:
      padding = 4;
      break;
    case StFl:
      padding = 8;
      break;
    default:
      return;
  }
  if (!((context->prelude.vars < address &&
         address + padding <
             context->prelude.vars + context->prelude.vars_size) ||
        (context->buffer + context->buffer_size > address &&
         address > context->buffer + context->prelude.static_size))) {
    invalid_access("[Error] Store outside of writable segments!\n");
  }
}
void call_instruction(Context *context, Instruction instruction) {
  write_be_bytes((byte *)&context->ip, 8,
                 context->buffer + context->registers[31]);
  context->registers[31] += 8;
  context->ip =
      context->buffer + *instruction.destination + instruction.immediate;
}
void return_instruction(Context *context) {
  context->registers[31] -= 8;
  u64 address = from_be_bytes(context->buffer + context->registers[31]);
  context->ip = context->buffer + address;
}
void interrupt_instruction(Context *context, Instruction instruction) {
  context->privileges = 1;
  u64 fake_register = *((u64 *)context->iht + *instruction.destination);
  Instruction fake_inst = {
      .destination = &fake_register,
      .immediate = 0,
      .functional = 0,
      .alt_source = 0,
      .source = 0,
  };
  call_instruction(context, fake_inst);
}
void interrupt_return_instruction(Context *context) {
  context->privileges = 0;
  return_instruction(context);
}

int exec_instruction(Context *context) {
  context->registers[0] = 0;
  struct Cancer cancer = decode_instruction(context);
  Instruction instruction = cancer.inst;
  eprintf("[LOG] Executing instruction: %04X\n", instruction.functional);
  eprintf("[LOG] Executing instruction: %s\n",
          get_funccode_name(instruction.functional));
  context->ip += cancer.sz;
  byte *sp = context->buffer + context->registers[31];
  switch (instruction.functional) {
    case Halt:
      return 0;
    case NoOp:
      break;
    case DIHT:
      todo();
      break;
    case SIHT:
      u64 tmp = (u64)context->iht - (u64)context->buffer;
      write_be_bytes((byte *)&tmp, 8, sp);
      write_be_bytes((byte *)&context->iht_size, 2, sp + 8);
      context->registers[31] += 10;
      break;
    case OIHT:
      context->iht = context->buffer + from_be_bytes(sp - 10);
      context->iht_size = from_be_bytes_16(sp - 2);
      context->registers[31] -= 10;
      break;
    case Retn:
      return_instruction(context);
      break;
    case Itrt:
      interrupt_instruction(context, instruction);
      break;
    case ItRt:
      interrupt_return_instruction(context);
      break;
    case Dupe:
      *instruction.destination = *instruction.source;
      break;
    case And:
      *(instruction.destination) &= *(instruction.source);
      break;
    case Or:
      *(instruction.destination) |= *(instruction.source);
      break;
    case Xor:
      *(instruction.destination) ^= *(instruction.source);
      break;
    case Not:
      *(instruction.destination) = ~*(instruction.destination);
      break;
    case Jump:
      jump_instruction(context, instruction);
      break;
    case JIfE:
      if (*instruction.destination == *instruction.source)
        jump_instruction(context, instruction);
      break;
    case JIfG:
      if (*instruction.destination > *instruction.source)
        jump_instruction(context, instruction);
      break;
    case JIfL:
      if (*instruction.destination < *instruction.source)
        jump_instruction(context, instruction);
      break;
    case JIGE:
      if (*instruction.destination >= *instruction.source)
        jump_instruction(context, instruction);
      break;
    case JILE:
      if (*instruction.destination <= *instruction.source)
        jump_instruction(context, instruction);
      break;
    case JINE:
      if (*instruction.destination != *instruction.source)
        jump_instruction(context, instruction);
      break;
    case MvSg:
      *instruction.destination &= 0xffffffffffff0000;
      *instruction.destination |= instruction.immediate;
      break;
    case MvDb:
      *instruction.destination &= 0xffffffff0000ffff;
      *instruction.destination |= instruction.immediate << 16;
      break;
    case MvQd:
      *instruction.destination &= 0xffff0000ffffffff;
      *instruction.destination |= instruction.immediate << 32;
      break;
    case MvFl:
      *instruction.destination &= 0x0000ffffffffffff;
      *instruction.destination |= instruction.immediate << 48;
      break;
    case LIHT:
      context->iht = (context->buffer + *instruction.destination);
      context->iht_size = instruction.immediate;
      if (*(context->iht + context->iht_size) != 0) {
        printf("[Error] Interrupt handler table is not ended with a 0-byte!\n");
        exit(0);
      }
      break;
    case Call:
      call_instruction(context, instruction);
      break;
    case LdSg:
      *instruction.destination =
          *(context->buffer + *instruction.source + instruction.immediate);
      break;
    case LdDb:
      *instruction.destination = from_be_bytes_16(
          context->buffer + *instruction.source + instruction.immediate);
      break;
    case LdQd:
      *instruction.destination = from_be_bytes_32(
          context->buffer + *instruction.source + instruction.immediate);
      break;
    case LdFl:
      *instruction.destination = from_be_bytes(
          context->buffer + *instruction.source + instruction.immediate);
      break;
    case StSg:
      store_instruction_check(context, instruction);
      *(u8 *)(context->buffer + *instruction.destination +
              instruction.immediate) = *(u8 *)instruction.source;
      break;
    case StDb:
      store_instruction_check(context, instruction);
      *(u16 *)(context->buffer + *instruction.destination +
               instruction.immediate) = *(u16 *)instruction.source;
      break;
    case StQd:
      store_instruction_check(context, instruction);
      *(u32 *)(context->buffer + *instruction.destination +
               instruction.immediate) = *(u32 *)instruction.source;
      break;
    case StFl:
      store_instruction_check(context, instruction);
      *(u64 *)(context->buffer + *instruction.destination +
               instruction.immediate) = *instruction.source;
      break;
    case Add:
      *instruction.destination = *instruction.destination +
                                 *instruction.source + instruction.immediate;
      break;
    case Sub:
      *instruction.destination -= *instruction.source;
      *instruction.destination -= instruction.immediate;
      break;
    case Put:
      printf("%s", (char *)(context->buffer + context->registers[1]));
      break;
  }
  return 1;
}
#endif