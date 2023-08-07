#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO:
// 1. WrFl
// 2. Make bofa them resolve the file size if the provided size is 0

static int IS_QUIET = 0;
#define error(msg, ...)                                                        \
  printf("[ERROR] ");                                                          \
  printf(msg, ##__VA_ARGS__);
#define log(msg, ...)                                                          \
  if (!IS_QUIET) {                                                             \
    fprintf(stderr, "[LOG]   ");                                               \
    fprintf(stderr, msg, ##__VA_ARGS__);                                       \
  }
#define log_ubr(msg, ...)                                                      \
  if (!IS_QUIET) {                                                             \
    fprintf(stderr, msg, ##__VA_ARGS__);                                       \
  }
#define str(symbol)                                                            \
  { symbol, #symbol }

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint8_t u8;

static enum { undetermined, little, big } ENDIANESS = undetermined;
void determine_endianess() {
  char stuff[] = {0x11, 0x22, 0x33, 0x44};
  u32 interpreted = *((u32 *)&stuff);
  if (interpreted == 0x11223344) {
    ENDIANESS = big;
    log("Endianess is big!\n");
  } else if (interpreted == 0x44332211) {
    ENDIANESS = little;
    log("Endianess is little!\n");
  } else {
    error("Could not determine endianess!\n");
    exit(0);
  }
}

void swap_bytes(int byte_count, void *bytes) {
  u8 *thing = bytes;
  u8 tmp = 0;
  for (int i = 0; i < byte_count / 2; i++) {
    tmp = thing[i];
    thing[i] = thing[byte_count - 1 - i];
    thing[byte_count - 1 - i] = tmp;
  }
}

static inline void to_big_endian(int byte_count, void *bytes) {
  if (ENDIANESS == little) {
    swap_bytes(byte_count, bytes);
  }
}

void from_big_endian(int byte_count, void *destination, void *source) {
  log("Read from little endian bytes <0x");
  for (int i = 0; i < byte_count; i++) {
    log_ubr("%02X", ((u8 *)source)[i]);
    ((u8 *)destination)[i] = ((u8 *)source)[i];
  }
  log_ubr(">!\n");
  to_big_endian(byte_count, destination);
}

void test_swap() {
  log("Testing byte swapping!\n");
  u16 test_number[3] = {0x1122, 0x3344, 0x5566};
  log("test_number is 0x112233445566!\n");
  swap_bytes(6, &test_number);
  log("test_number is 0x%X%X%X!\n", test_number[0], test_number[1],
      test_number[2]);
  if (test_number[0] != 0x6655) {
    error("Swapping bytes test failed!\n");
    exit(0);
  }
}

FILE *get_file(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Correct usage:\n\tturnstile <filename.stile>\n");
    exit(0);
  }
  FILE *out = fopen(argv[1], "r");
  if (out == NULL) {
    switch (errno) {
    case 2:
      error("File <%s> does not exist!\n", argv[1]);
      exit(0);
    default:
      error("Encountered error while opening file!\n");
      exit(0);
    }
  }
  log("File <%s> opened!\n", argv[1]);
  return out;
}

typedef struct {
  u64 registers[32];
  u64 stack_pointer;
  u64 instruction_pointer;
  u64 *iht_pointer;
  u8 *memory;
  u64 static_size;
  u64 memory_size;
} Context;

#define INSTMASK 0b1111111100000110
typedef enum {
#define LMASK = 0b0000000000000000,
  Halt = 0b0000000000000000,
  NoOp = 0b0000000000000010,
  LdSg = 0b0000000100000000,
  LdDb = 0b0000000100000010,
  LdQd = 0b0000000100000100,
  LdOc = 0b0000000100000110,
  SxSg = 0b0000001000000000,
  SxDb = 0b0000001000000010,
  SxQd = 0b0000001000000100,
  Intr = 0b0000001100000000,
  LIHT = 0b0000010000000000,

#define TMASK = 0b0010000000000000,
  Dupe = 0b0010000000000000,
  bAnd = 0b0010000100000000,
  bOr = 0b0010000100000010,
  bXor = 0b0010000100000100,
  bNot = 0b0010000100000110,
  ShL = 0b0010001000000000,
  ShRZ = 0b0010001000000010,
  ShRO = 0b0010001000000110,
  RdSg = 0b0010001100000000,
  RdDb = 0b0010001100000010,
  RdQd = 0b0010001100000100,
  RdOc = 0b0010001100000110,
  StSg = 0b0010010000000000,
  StDb = 0b0010010000000010,
  StQd = 0b0010010000000100,
  StOc = 0b0010010000000110,
  Add = 0b0010010100000000,
  Sub = 0b0010010100000010,
  Mul = 0b0010010100000100,
  Div = 0b0010010100000110,

#define CMASK = 0b0100000000000000,
  Jump = 0b0100000000000000,
  JIfE = 0b0100000100000000,
  JINE = 0b0100000100000010,
  JIfG = 0b0100001000000000,
  JIGE = 0b0100001000000010,
  JIfL = 0b0100001100000000,
  JILE = 0b0100001100000010,

#define VMASK = 0b0110000000000000,
  Put = 0b0110010000000000,
  PReg = 0b0110010100000000,
  LdFl = 0b0110011000000000,
} Opcode;

char *opcode_name(Opcode opcode) {
  const struct {
    Opcode opcode;
    char *name;
  } assoc[] = {
      str(Halt), str(NoOp), str(LdSg), str(LdDb), str(LdQd), str(LdOc),
      str(SxSg), str(SxDb), str(SxQd), str(Dupe), str(bAnd), str(bOr),
      str(bXor), str(bNot), str(ShL),  str(ShRZ), str(ShRO), str(RdSg),
      str(RdDb), str(RdQd), str(RdOc), str(StSg), str(StDb), str(StQd),
      str(StOc), str(Add),  str(Sub),  str(Mul),  str(Div),  str(Jump),
      str(JIfE), str(JINE), str(JIfG), str(JIGE), str(JIfL), str(JILE),
      str(Put),  str(PReg), str(Intr), str(LIHT), str(LdFl), {0, "undefined"}};
  long long unsigned i;
  for (i = 0; i < (sizeof(assoc) / sizeof(assoc[0]) - 1); i++) {
    if (opcode == assoc[i].opcode) {
      break;
    }
  }
  return assoc[i].name;
}

typedef struct {
  Opcode opcode;
  u64 *destination;
  u64 *source;
  u64 *additional_source;
  u64 *return_register;
  u32 immediate;
} Instruction;

Instruction decode(Context *ctx) {
  u32 raw = 0;
  from_big_endian(4, &raw, ctx->memory + ctx->instruction_pointer);
  log("Decoding instruction from <0x%08X>!\n", raw);
  Opcode opcode = (raw >> 16) & INSTMASK;
  log("Opcode is <0x%04X>!\n", opcode);
  u64 *destination = &ctx->registers[0b11111 & (raw >> 19)],
      *source = &ctx->registers[0b11111 & (raw >> 12)],
      *additional_source = &ctx->registers[0b11111 & raw],
      *return_register = &ctx->registers[0b11111 & (raw >> 7)];
  u32 immediate;
  switch (raw >> 29) {
  case 0b000:
    immediate = 0xFFFF & raw;
    break;
  case 0b001:
    immediate = 0xFFF & raw;
    break;
  case 0b010:
    immediate = 0;
    break;
  case 0b011:
    immediate = 0xFFFFFF & raw;
    break;
  default:
    error("Unsupported instruction format <%i>!\n", raw >> 29);
    exit(0);
  };
  return (Instruction){
      .additional_source = additional_source,
      .return_register = return_register,
      .destination = destination,
      .immediate = immediate,
      .opcode = opcode,
      .source = source,
  };
}

void print_registers(Context *ctx, u32 i) {
  int r = 0b11111 & i;
  printf("r%i: %lli\n", r, ctx->registers[r]);
}

void call_interrupt(Context *ctx, u64 *reg) {
  if (ctx->iht_pointer == 0) {
    error("Tried calling an interrupt with invalid IHT!\n")
  } else {
    u64 address = 0;
    from_big_endian(4, &address, ctx->iht_pointer + *reg);
    ctx->stack_pointer -= 8;
    from_big_endian(8, ctx->memory + ctx->stack_pointer,
                    &ctx->instruction_pointer);
    ctx->instruction_pointer = address;
  }
}

void load_file(Context* ctx, u64 i) {
  u8 destination_register = (i >> 16) & 0b00011111;
  u8 count_register = (i >> 8) & 0b00011111;
  u8 path_register = i & 0b00011111;
  char* path = (char*) ctx->memory + ctx->registers[path_register];
  FILE* f = fopen(path, "r");
  if (f == NULL) {
    error("Could not LdFl on <%s>!\n", path);	
    error("Errno code is <%i>!\n", errno);
    exit(0);
  }
  fread(ctx->memory + ctx->registers[destination_register], 1, ctx->registers[count_register], f);
  fclose(f);
}

int execute(Context *ctx, Instruction instruction) {
  log("Executing instruction at <%llX>!\n", ctx->instruction_pointer);
  log("Instruction is <%s>!\n", opcode_name(instruction.opcode));
  ctx->registers[0] = 0;
  ctx->instruction_pointer += 4;
  switch (instruction.opcode) {
  case Halt:
    return 0;
  case NoOp:
    break;
  case LdSg:
    *instruction.destination &= 0xFFFFFFFFFFFF0000;
    *instruction.destination |= (u16)instruction.immediate;
    break;
  case LdDb:
    *instruction.destination &= 0xFFFFFFFF0000FFFF;
    *instruction.destination |= (u32)(u16)instruction.immediate << 16;
    break;
  case LdQd:
    *instruction.destination &= 0xFFFF0000FFFFFFFF;
    *instruction.destination |= (u64)(u16)instruction.immediate << 32;
    break;
  case LdOc:
    *instruction.destination &= 0x0000FFFFFFFFFFFF;
    *instruction.destination |= (u64)(u16)instruction.immediate << 48;
    break;
  case SxSg:
    if (*instruction.destination & (1 << 15))
      *instruction.destination |= 0xFFFFFFFFFFFF0000;
    break;
  case SxDb:
    if (*instruction.destination & (1 << 31))
      *instruction.destination |= 0xFFFFFFFF00000000;
    break;
  case SxQd:
    if (*instruction.destination & (1LL << 47))
      *instruction.destination |= 0xFFFF000000000000;
    break;
  case Intr:
    call_interrupt(ctx, instruction.destination);
    break;
  case LIHT:
    ctx->iht_pointer = (u64 *)(ctx->memory + *instruction.destination);
    break;
  case Dupe:
    *instruction.destination = *instruction.source;
    break;
  case bAnd:
    *instruction.destination &= *instruction.source;
    break;
  case bOr:
    *instruction.destination |= *instruction.source;
    break;
  case bXor:
    *instruction.destination ^= *instruction.source;
    break;
  case bNot:
    *instruction.destination = ~*instruction.source;
    break;
  case ShL:
    *instruction.destination <<= *instruction.source + instruction.immediate;
    break;
  case ShRZ:
    *instruction.destination >>= *instruction.source + instruction.immediate;
    *instruction.destination &=
        0xFFFFFFFFFFFFFFFFu >> (*instruction.source + instruction.immediate);
    break;
  case ShRO:
    *instruction.destination >>= *instruction.source + instruction.immediate;
    *instruction.destination |=
        ~(0xFFFFFFFFFFFFFFFFu >> (*instruction.source + instruction.immediate));
    break;
  case RdSg:
    from_big_endian(1, instruction.destination,
                    ctx->memory + *instruction.source +
                        (i16)instruction.immediate);
    break;
  case RdDb:
    from_big_endian(2, instruction.destination,
                    ctx->memory + *instruction.source +
                        (i16)instruction.immediate);
    break;
  case RdQd:
    from_big_endian(4, instruction.destination,
                    ctx->memory + *instruction.source +
                        (i16)instruction.immediate);
    break;
  case RdOc:
    from_big_endian(8, instruction.destination,
                    ctx->memory + *instruction.source +
                        (i16)instruction.immediate);
    break;
  case StSg:
    from_big_endian(
        1, ctx->memory + *instruction.destination + (i16)instruction.immediate,
        instruction.source);
    break;
  case StDb:
    from_big_endian(
        2, ctx->memory + *instruction.destination + (i16)instruction.immediate,
        instruction.source);
    break;
  case StQd:
    from_big_endian(
        4, ctx->memory + *instruction.destination + (i16)instruction.immediate,
        instruction.source);
    break;
  case StOc:
    from_big_endian(
        8, ctx->memory + *instruction.destination + (i16)instruction.immediate,
        instruction.source);
    break;
  case Add:
    *instruction.destination += *instruction.source + instruction.immediate;
    break;
  case Sub:
    *instruction.destination -= *instruction.source - instruction.immediate;
    break;
  case Mul:
    *instruction.destination *=
        *instruction.source + (i16)instruction.immediate;
    break;
  case Div:
    *instruction.destination /=
        *instruction.source + (i16)instruction.immediate;
    break;
  case Jump:
    *instruction.return_register = ctx->instruction_pointer;
    ctx->instruction_pointer = *instruction.additional_source;
    break;
  case JIfE:
    if (*instruction.destination == *instruction.source) {
      *instruction.return_register = ctx->instruction_pointer;
      ctx->instruction_pointer = *instruction.additional_source;
    }
    break;
  case JINE:
    if (*instruction.destination != *instruction.source) {
      *instruction.return_register = ctx->instruction_pointer;
      ctx->instruction_pointer = *instruction.additional_source;
    }
    break;
  case JIfG:
    if (*instruction.destination > *instruction.source) {
      *instruction.return_register = ctx->instruction_pointer;
      ctx->instruction_pointer = *instruction.additional_source;
    }
    break;
  case JIGE:
    if (*instruction.destination >= *instruction.source) {
      *instruction.return_register = ctx->instruction_pointer;
      ctx->instruction_pointer = *instruction.additional_source;
    }
    break;
  case JIfL:
    if (*instruction.destination < *instruction.source) {
      *instruction.return_register = ctx->instruction_pointer;
      ctx->instruction_pointer = *instruction.additional_source;
    }
    break;
  case JILE:
    if (*instruction.destination <= *instruction.source) {
      *instruction.return_register = ctx->instruction_pointer;
      ctx->instruction_pointer = *instruction.additional_source;
    }
    break;
  case Put:
    printf("%.*s", instruction.immediate, ctx->memory + ctx->registers[1]);
    break;
  case PReg:
    print_registers(ctx, instruction.immediate);
    break;
  case LdFl:
    load_file(ctx, instruction.immediate);
    break;
  }
  return 1;
}

#define MEMSZ 4096
void start(int argc, char *argv[]) {
  FILE *file = get_file(argc, argv);
  log("Initializing context with %i bytes of memory!\n", MEMSZ);
  Context ctx = {.registers = {0},
                 .stack_pointer = MEMSZ,
                 .iht_pointer = 0,
                 .instruction_pointer = 0,
                 .memory = malloc(MEMSZ),
                 .memory_size = MEMSZ};
  log("Reading file's contents into memory!\n");
  int i = fread(ctx.memory, 1, MEMSZ, file);
  log("Read %i bytes!\n", i);
  ctx.static_size = i;
  log("Memory dump:\n");
  log_ubr("00000000: ");
  for (int i = 0; i < MEMSZ; i++) {
    if (i % 16 == 0 && i > 0) {
      log_ubr("\n");
      log_ubr("%08X: ", i);
    } else if (i % 2 == 0) {
      log_ubr(" ");
    }
    log_ubr("%02X", ctx.memory[i]);
  }
  log_ubr("\n");
  for (Instruction instruction = decode(&ctx); execute(&ctx, instruction);
       instruction = decode(&ctx)) {
  }
}

int main(int argc, char **argv) {
  if (argc > 1 && strncmp(argv[1], "-q", 2) == 0) {
    IS_QUIET = 1;
    argc--;
    argv++;
  }
  determine_endianess();
  test_swap();
  start(argc, argv);
  return 0;
}
