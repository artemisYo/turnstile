#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define error(msg, ...) \
  printf("[ERROR] ");   \
  printf(msg, ##__VA_ARGS__);
#define log(msg, ...)          \
  fprintf(stderr, "[LOG]   "); \
  fprintf(stderr, msg, ##__VA_ARGS__);

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

static enum { undetermined, little, big } ENDIANESS = undetermined;
void determine_endianess() {
  char stuff[] = {0x11, 0x22, 0x33, 0x44};
  u32 interpreted = *((u32*)&stuff);
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

void swap_bytes(int byte_count, void* bytes) {
  u8* thing = bytes;
  u8 tmp = 0;
  for (int i = 0; i < byte_count / 2; i++) {
    tmp = thing[i];
    thing[i] = thing[byte_count - 1 - i];
    thing[byte_count - 1 - i] = tmp;
  }
}

static inline void to_big_endian(int byte_count, void* bytes) {
  if (ENDIANESS == little) {
    swap_bytes(byte_count, bytes);
  }
}

void as_big_endian(int byte_count, void* destination, void* source) {
  for (int i = 0; i < byte_count; i++) {
    ((u8*)destination)[i] = ((u8*)source)[i];
  }
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

FILE* get_file(int argc, char* argv[]) {
  if (argc != 2) {
    printf("Correct usage:\n\tturnstile <filename.stile>\n");
    exit(0);
  }
  FILE* out = fopen(argv[1], "r");
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
  u64 registers[30];
  u64 stack_pointer;
  u64 memory_pointer;
  u64 func_table_pointer;
  u64 instruction_pointer;
  u8* memory;
  u64 static_size;
  u64 memory_size;
} Context;

#define INSTMASK 0b1111111100000110
typedef enum {
  LMASK = 0b0000000000000000,
  Halt = 0b0000000000000000,
  NoOp = 0b0000000000000010,
  LdSg = 0b0000000100000000,
  LdDb = 0b0000000100000010,
  LdQd = 0b0000000100000100,
  LdOc = 0b0000000100000110,
  Call = 0b0000001000000000,
  Retn = 0b0000001000000010,
  LFT = 0b0000001100000000,

  TMASK = 0b0010000000000000,
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

  CMASK = 0b0100000000000000,
  Jump = 0b0100000000000000,
  JIfE = 0b0100000100000000,
  JINE = 0b0100000100000010,
  JIfG = 0b0100001000000000,
  JIGE = 0b0100001000000010,
  JIfL = 0b0100001100000000,
  JILE = 0b0100001100000010,

  VMASK = 0b0110000000000000,
  Put = 0b0110010000000000,
} Opcode;

typedef struct {
  Opcode opcode;
  u64* destination;
  u64* source;
  u64* additional_source;
  u32 immediate;
} Instruction;

Instruction decode(Context* ctx) {
  u32 raw = 0;
  as_big_endian(4, &raw, ctx->memory + ctx->instruction_pointer);
  Opcode opcode = raw & INSTMASK;
  u64 *destination = &ctx->registers[0b11111 & (raw >> 19)],
      *source = &ctx->registers[0b11111 & (raw >> 12)],
      *additional_source = &ctx->registers[0b11111 & raw];
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
      .destination = destination,
      .immediate = immediate,
      .opcode = opcode,
      .source = source,
  };
}

#define MEMSZ 4096
void start(int argc, char* argv[]) {
  FILE* file = get_file(argc, argv);
  log("Initializing context with %i bytes of memory!\n", MEMSZ);
  Context ctx = {.registers = {0},
                 .stack_pointer = MEMSZ - 1,
                 .memory_pointer = 0,
                 .func_table_pointer = 0,
                 .instruction_pointer = 0,
                 .memory = malloc(MEMSZ),
                 .memory_size = MEMSZ};
  log("Reading file's contents into memory!\n");
  int i = 0;
  for (int c = fgetc(file); i < MEMSZ && c != EOF; c = fgetc(file), i++) {
    ctx.memory[i] = (u8)c;
  }
  log("Read %i bytes!\n", i);
  ctx.static_size = i;
  while (1) {
    Instruction instruction = decode(&ctx);
  }
}

int main(int argc, char* argv[]) {
  determine_endianess();
  test_swap();
  start(argc, argv);
  return 0;
}
