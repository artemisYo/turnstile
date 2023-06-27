This is a doc specifying turnstile's behaviour.
# Stuff idk
Turnstile has 31 general use registers, all of which are 64-bits wide.
Other registers include a Comparison flag register, 
which is subdivided into flags like `CE` comparison equal 
or `CG` comparison greater than.
There is also `ip` which keeps track of the current executor position and is
modified by jump instructions.
Additionally `sp` should be used to keep track of the stack.
| Register | Register Code | 0bRegCode | Description      |
| -------- | ------------- | --------- | ---------------- |
| r00-31   |          0-31 |  0b000000 | General Purpose  |
| sp       |            32 |  0b111111 | Stack management |
| ip       |          None |      None | Branching        |
| CmpReg   |          None |      None | Branching        |
# Bytecode
First and foremost the encoding: turnstile is big-endian encoded.
The following is the encoding of an instruction:
R: register 1, destination, 5 bits
r: register 2, source, 5 bits
i: immediate, offset or normal value, 12 bits
f: functional, select instruction from group, 3 bits
o: opcode, 7 bits
|RRRR_Rrrr|rrii_iiii|iiii_iiff|fooo_oooo|
Or in words: each instruction is 32 bits long,
the most significant 5 bits denote the destination register,
the 5 bits following denote sourcre register, they may be ignored,
the 12 bits following that are an immediate value,
the next 3 bits select an instruction in a group,
the 7 least significant bits are the opcode.
## Opcodes
| Name | Functional | Opcode | 0xOpcode | 0bfffOOOOOOO | Description           |
| ---- | ---------- | ------ | -------- | ------------ | --------------------- |
| Halt |          0 |      0 |     0x00 | 0b0000000000 | Halts execution       |
| NoOp |          1 |      0 |          | 0b0010000000 | Does nothing          |
| Inc  |          0 |     16 |     0xa0 | 0b0000010000 | R += 1                |
| Dec  |          1 |        |          | 0b0010010000 | R -= 1                |
| Add  |          0 |     17 |     0xa1 | 0b0000010001 | R += r + i            |
| Sub  |          1 |     17 |          | 0b0010010001 | R += -r - i           |
| Jmp  |          0 |     18 |     0xa3 | 0b0000010010 | ip = r + i            |
| JIfE |          1 |        |          | 0b0010010010 | ip = CE ? r + i : ip  |
| JIfG |          2 |        |          | 0b0100010010 | ip = CG ? r + i : ip  |
| JIfL |          3 |        |          | 0b0110010010 | ip = CL ? r + i : ip  |
| JIGE |          4 |        |          | 0b1000010010 | ip = !CL ? r + i : ip |
| JILE |          5 |        |          | 0b1010010010 | ip = !CG ? r + i : ip |
| JINE |          6 |        |          | 0b1100010010 | ip = !CE ? r + i : ip |
| Cmp  |          7 |        |          | 0b1110010010 | Sets CE, CG and CL    |
| Mov  |            |     19 |     0xa4 | 0b0000010011 | R = r                 |
| StrN |          0 |     20 |     0xa5 | 0b0000010100 | R = i                 |
| StrO |          1 |        |          | 0b0010010100 | R = i << 12           |
| StrD |          2 |        |          | 0b0100010100 | R = i << 24           |
| StrT |          3 |        |          | 0b0110010100 | R = i << 36           |
| StrQ |          4 |        |          | 0b1000010100 | R = i << 48           |
| StrF |          5 |        |          | 0b1010010100 | R = (i & 0x0f) << 60  |

# Layout
Turnstile's bytecode has to declare an entry point, 
it is a 64-bit number reffering to an offset from the start,
to which the executor jumps, but if the integer is 0,
the executor simply jumps to the start of the code segment.
Following the entry point is the location and size of some segments (meta),
which are similar to `.data` and other segments present on some architectures.
A non-existent segment's meta does not need to be declared.
Each segment's meta is declared by a byte denoting the id of the segment,
as well as two 64-bit numbers, 
first is another offset denoting the segment, 
second is an integer type denoting the size of the segment.
Since each segment's size is the same, they follow eachother directly, 
after all metas are listed a null byte follows. 
These declarations form the prelude of the bytecode file.
## Segments
| Name | Id | 0xId | Description              |
| ---- | -- | ---- | ------------------------ |
| Code | 10 | 0xa0 | Contains executable code |
| Data | 11 | 0xa1 | Contains constant data   |
| Vars | 12 | 0xa2 | Contains writable data   |