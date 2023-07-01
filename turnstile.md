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
| None:       |    8 | 100f_ffff |           |           |           | Be   |
| R:          |    8 | 11fR_RRRR |           |           |           | Br   |
| R, r:       |   16 | 000R_RRRR | rrrr_rfff |           |           | D    |
| R, i:       |   32 | 001R_RRRR | f0ff_f0ff | iiii_iiii | iiii_iiii | Qi   |
| R, r, i:    |   32 | 001R_RRRR | rrrr_r1ff | ffff_iiii | iiii_iiii | Qo   |
| R, r, s, i: |   32 | 010R_RRRR | rrrr_rfff | fffs_ssss | iiii_iiii | Qf   |
| ----------- | ---- | --------- | --------- | --------- | --------- | ---- |
| Virtual Extension, these are instructions to interact with the execution  |
| environment in a vm                                                       |
| ----------- | ---- | --------- | --------- | --------- | --------- | ---- |
| Any         |    8 | 101f_ffff |           |           |           | V    |
| ----------- | ---- | --------- | --------- | --------- | --------- | ---- |
| The encodings beneath should be used sparingly, as they would be slow to  |
| decode and they are quite big                                             |
| ----------- | ---- | --------- | --------- | --------- | --------- | ---- |
| R, i:       |   80 | 011R_RRRR | f0ff_f0ff |           |           | Dpi  |
|             |      | iiii_iiii | iiii_iiii | iiii_iiii | iiii_iiii |      |
|             |      | iiii_iiii | iiii_iiii | iiii_iiii | iiii_iiii |      |

The above are the format patterns for different instruction types.
In the pattern letters stand for a bit of the corresponding argument.
The letter `f` signifies a bit of the functional code, this identifies
the actual operation to be done. The functional code is split in some 
cases, this is to keep the parity of the rest of the data.

| Name | Description                        | f/max_f         | Privileges? |
| ---- | ---------------------------------- | --------------- | ----------- |
| No argument                                                               |
| ---- | ---------------------------------- | --------------- | ----------- |
| Halt | Halts execution                    | 0/31            | No          |
| NoOp | Does nothing                       | 1/31            | No          |
| DIHT | Resets IHT                         | 2/31            | Yes         |
| SIHT | Pushes IHT onto stack              | 3/31            | Yes         |
| OIHT | Pops IHT from stack                | 4/31            | Yes         |
| Retn | Pops ip from stack and returns     | 5/31            | No          |
| ---- | ---------------------------------- | --------------- | ----------- |
| 5 bit register or none                                                    |
| ---- | ---------------------------------- | --------------- | ----------- |
| Itrt | Interrupt with R as handler id     | 0/1             | No          |
| ItRt | Return from interrupt              | 1/1             | No          |
| ---- | ---------------------------------- | --------------- | ----------- | 
| 5 bit source and destination register args                                | 
| ---- | ---------------------------------- | --------------- | ----------- | 
| Dupe | R = r                              | 2/7             | No          | 
| And  | R &= r                             | 3/7             | No          | 
| Or   | R |= r                             | 4/7             | No          | 
| Xor  | R ^= r                             | 5/7             | No          |
| Not  | R = !r                             | 6/7             | No          |
| ---- | ---------------------------------- | --------------- | ----------- | 
| 5 bit lhs, rhs and src register, 8 bit offset                             | 
| ---- | ---------------------------------- | --------------- | ----------- | 
| Jump | ip = s + i                         | 0/63            | No          | 
| JIfE | ip = R==r ? s + i : ip             | 1/63            | No          | 
| JIfG | ip = R> r ? s + i : ip             | 2/63            | No          | 
| JIfL | ip = R< r ? s + i : ip             | 3/63            | No          | 
| JIGE | ip = R>=r ? s + i : ip             | 4/63            | No          | 
| JILE | ip = R<=r ? s + i : ip             | 5/63            | No          | 
| JINE | ip = R!=r ? s + i : ip             | 6/63            | No          | 
| ---- | ---------------------------------- | --------------- | ----------- | 
| 5 bit destination register, 16 bit immediate                              | 
| ---- | ---------------------------------- | --------------- | ----------- | 
| MvSg | R = i                              | 0/63            | No          | 
| MvDb | R = i << 16                        | 1/63            | No          | 
| MvQd | R = i << 32                        | 2/63            | No          | 
| MvFl | R = i << 48                        | 3/63            | No          | 
| LIHT | IHT = *R, IHTsz = i                | 4/63            | Yes         |
| Call | sp += 8, [sp - 8] = ip, ip = R + i | 5/63            | No          |
| ---- | ---------------------------------- | --------------- | ----------- |
| 5 bit dest and src register, 12 bit offset                                |
| ---- | ---------------------------------- | --------------- | ----------- |
| LdSg | R = (u8) [r + i]                   | 0/63            | No          |
| LdDb | R = (u16)[r + i]                   | 1/63            | No          |
| LdQd | R = (u32)[r + i]                   | 2/63            | No          |
| LdFl | R = (u64)[r + i]                   | 3/63            | No          |
| StSg | [R + i] = (u8) r                   | 4/63            | No          |
| StDb | [R + i] = (u16)r                   | 5/63            | No          |
| StQd | [R + i] = (u32)r                   | 6/63            | No          |
| StFl | [R + i] = (u64)r                   | 7/63            | No          |
| Add  | R = R + r + i                      | 8/63            | No          |
| Sub  | R = R - r - i                      | 9/63            | No          |
| ---- | ---------------------------------- | --------------- | ----------- |
| Any arguments, vm extension                                               |
| ---- | ---------------------------------- | --------------- | ----------- |
| Put  | Print c-string at address in r1    | 0/31            | No          |
| ---- | ---------------------------------- | --------------- | ----------- |
| 5 bit dest, 64 bit immediate                                              |
| ---- | ---------------------------------- | --------------- | ----------- |
|      |                                    |                 |             |

# Interrupt Handler Table
The IHT is a table embeded in the binary, which contains a list of offsets
to interrupt handlers, which get used to call the handler specified by the
index when `Itrt` is called. 
Upon start a default table is loaded, however a custom table can be loaded
using `LIHT`, which takes the address of the table through a register and
it's size through the immediate argument.
The table needs to be ended by a null byte.
The default table can be loaded using `DIHT`.
Since sometimes it might be useful to temporarily load the default IHT and
then restore the custom IHT, the instructions `SIHT` and `OIHT` exist.
`SIHT` may also be used as an alternative to `LIHT`, which supports dynamic
size arguments.
All interrupt instruction, except for `Itrt`, require privileges, this is a 
flag which is set at startup and when `Itrt` is called, it is unset when an
interrupt handler returns through `ItRt`.

# Layout
Turnstile's bytecode starts with a number determining it's
byte-length as a 64-bit number.
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