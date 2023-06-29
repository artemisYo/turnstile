This is a doc specifying turnstile's behaviour.
# Stuff idk
Turnstile has 30 general use registers, all of which are 64-bits wide.
There is also `ip` which keeps track of the current executor position and is
modified by jump instructions.
Additionally `sp` should be used to keep track of the stack
and `rZ` is always zero.
| Register | Register Code | 0bRegCode | Description      |
| -------- | ------------- | --------- | ---------------- |
| rZ       |             0 |   0b00000 | Always zero      |
| r01-31   |          1-30 |   0b00001 | General Purpose  |
| sp       |            31 |   0b11111 | Stack management |
| ip       |          None |      None | Branching        |
# Bytecode
First and foremost the encoding: turnstile is big-endian encoded.
Additionally the code segment of the bytecode has to be followed
by a padding of 8 Halt instructions.
## Instructions
Formats:
| Arguments   | size | 1st byte  | 2nd byte  | 3rd byte  | 4th byte  | Type |
| ----------- | ---- | --------- | --------- | --------- | --------- | ---- |
| None/R:     |    8 | 1ffR_RRRR |           |           |           | B    |
| R, r:       |   16 | 000R_RRRR | rrrr_rfff |           |           | D    |
| R, i:       |   32 | 001R_RRRR | f0ff_f0ff | iiii_iiii | iiii_iiii | Qi   |
| R, r, i:    |   32 | 001R_RRRR | rrrr_r1ff | ffff_iiii | iiii_iiii | Qo   |
| R, r, s, i: |   32 | 010R_RRRR | rrrr_rfff | fffs_ssss | iiii_iiii | Qf   |

The above are the format patterns for different instruction types.
In the pattern letters stand for a bit of the corresponding argument.
The letter `f` signifies a bit of the functional code, this identifies
the actual operation to be done. The functional code is split in some 
cases, this is to keep the parity of the rest of the data.
The prefix of `0b011x_xxxx` is reserved for longer instructions.

| Name | Description            | f/max_f         |
| ---- | ---------------------- | --------------- |
| 5 bit register or no argument                   |
| ---- | ---------------------- | --------------- |
| Halt | Halts execution        | 0/3             |
| NoOp | Does nothing           | 1/3             |
| Not  | R = !R                 | 2/3             |
| ---- | ---------------------- | --------------- |
| 5 bit source and destination register args      |
| ---- | ---------------------- | --------------- |
| Dupe | R = r                  | 2/7             |
| And  | R &= r                 | 3/7             |
| Or   | R |= r                 | 4/7             |
| Xor  | R ^= r                 | 5/7             |
| ---- | ---------------------- | --------------- |
| 5 bit lhs, rhs and src register, 8 bit offset   |
| ---- | ---------------------- | --------------- |
| Jump | ip = s + i             | 0/63            |
| JIfE | ip = R==r ? s + i : ip | 1/63            |
| JIfG | ip = R> r ? s + i : ip | 2/63            |
| JIfL | ip = R< r ? s + i : ip | 3/63            |
| JIGE | ip = R>=r ? s + i : ip | 4/63            |
| JILE | ip = R<=r ? s + i : ip | 5/63            |
| JINE | ip = R!=r ? s + i : ip | 6/63            |
| ---- | ---------------------- | --------------- |
| 5 bit destination register, 16 bit immediate    |
| ---- | ---------------------- | --------------- |
| MvSg | R = i                  | 0/63            |
| MvDb | R = i << 16            | 1/63            |
| MvQd | R = i << 32            | 2/63            |
| MvFl | R = i << 48            | 3/63            |
| ---- | ---------------------- | --------------- |
| 5 bit dest and src register, 12 bit offset      |
| ---- | ---------------------- | --------------- |
| LdSg | R = (u8) [r + i]       | 0/63            |
| LdDb | R = (u16)[r + i]       | 1/63            |
| LdQd | R = (u32)[r + i]       | 2/63            |
| LdFl | R = (u64)[r + i]       | 3/63            |
| StSg | [R + i] = (u8) r       | 4/63            |
| StDb | [R + i] = (u16)r       | 5/63            |
| StQd | [R + i] = (u32)r       | 6/63            |
| StFl | [R + i] = (u64)r       | 7/63            |
| Add  | R = R + r + i          | 8/63            |
| Sub  | R = R - r - i          | 9/63            |


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