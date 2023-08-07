#set par(justify: true)
#set text(font: "IBM Plex Sans")
#let title(body) = {
  set text(size: 20pt);
  align(center, [#heading(level: 1, body)])
}
#let section(skip: true, level: 2,title) = {
  if skip {v(5%)}; 
  heading(level: level, title)
}
#let subsection(title) = section(skip: false, level: 3, title)
#let instruction_section(title) = {pagebreak(); subsection(title)}
#let part(title) = heading(outlined: false, level: 4, [#h(10%); #title])
#let ref_section(name) = {
  text(weight: "bold", name)
}
#let code(c) = {linebreak(); raw(c); linebreak(); linebreak()}
#show figure: it => align(center)[
  #it.caption
  #it.body
]
#show table: it => [#set text(font: "IBM Plex Mono"); #it]
#outline(indent: 3%)
#pagebreak()
#title[Turnstile Spec]
#section(skip: false)[Instruction Formats]
Turnstile's machine-/bytecode is made up of mostly 32bit instructions,
however future extension to bigger sizes is possible.
Turnsile's bytecode is encoded in big-endian.
The instructions are mostly grouped into 3 formats and one additional format for virtual execution environments.
The 3 basic formats are structured as such:
#figure(
  caption: [Basic Formats],
  table(
    align: (left, center, right, right, right, right),
    columns: (0.8fr, 0.4fr, 1fr, 1fr, 1fr, 1fr),
    
    [Args],       [Type], [1st Byte],  [2nd Byte],  [3rd Byte],  [4th Byte],
    [R, i],       [L],    [000o oooo], [RRRR Rff0], [iiii iiii], [iiii iiii],
    [R, r, i],    [T],    [001o oooo], [RRRR Rffr], [rrrr iiii], [iiii iiii],
    [R, r, s, a], [C],    [010o oooo], [RRRR Rffr], [rrrr aaaa], [a00s ssss],
  )
)<basic_formats>

#figure(
  caption: [Virtual Format],
  table(
    align: (left, center, right, right, right, right),
    columns: (0.8fr, 0.4fr, 1fr, 1fr, 1fr, 1fr),
    
    [i], [V], [011o oooo], [iiii iiii], [iiii iiii], [iiii iiii],
  )
)<virtual_formats>
The seemingly random letters signify the bits of certain values,
the letters used also for arguments, denote each bit of the arguments value.
The letters which are not arguments however, have the following meaning: 
the letter ~o~ denotes the opcode of the instruction, 
it is an identifier for instructions and cannot be ~1 1111~;
the letter ~f~ denotes the functional group, 
it is used for instructions which should be grouped, 
such as ~Add~ and ~Sub~, 
this number may be treated the same as additional bit to the opcode.

R~ is usually called the destination register,
~r~ is the source register,
~s~ is the additional source register and
~a~ is the return register.
~i~ is the immediate.

Certain instruction formats are designed for specific instruction types. 
Type L was made for (l)oading data,
i.e. any instruction which firstly pertains to the immediate argument.
On the other hand type T is used for data (t)ransfer,
it's purpose is to store instructions that load data from memory, 
but also for instructions that deal with data in registers,
such as the aforementioned ~Add~ instruction.
Type C was made for (c)ontrol flow, such as the ~Jump~ instruction,
it deals with 4 registers, 
the reason for which will become apparent 
when conditional branching instructions are introduced.

/*
#section[Microcode]
Microcode is chosen for the design of turnstile, 
as it allows a more structured approach to implementing instructions.
As microcode is generally hard to nail down, the current version is only a draft.
the microcode will be parametrized in some cases, 
this is means that operations like ~enable destination register~ 
will not be implemented by selecting the microcode for the correct register,
but by the register being selected automatically by the register file 
and enabled at the correct position.
#pagebreak()
#figure(
  caption: "Instruction Decoder Microcodes",
  table(
    align: (left, left, center),
    columns: (0.2fr, 1fr, 0.4fr),
    [Name], [Description], [Correspondence],
    [DstO], [Output from destination register], [R],
    [SrcO], [Output from source register], [r],
    [ASrcO], [Output from additional source register], [s],
    [ImmO], [Output from immediate], [i],
  )
) <microcodes_instruction_decoder>
#figure(
  caption: "Instruction Pointer Microcodes",
  table(
    align: (left, left, center),
    columns: (0.2fr, 1fr, 0.4fr),
    [Name], [Description], [Correspondence],
    [IpSC], [Load jump instruction conditional], [Cmp R r],
    [IpSA], [Jump if conditional is true], [Jmp s],
    [IpI], [Increment instruction pointer], [ip++],
    [IpO], [Output instruction pointer onto bus], [ip],
  )
) <microcodes_instruction_pointer>
#figure(
  caption: "ALU Microcodes",
  table(
    align: (left, left, center),
    columns: (0.2fr, 1fr, 0.4fr),
    [Name], [Description], [Correspondence],
    [AlO], [Output result of calculation], [ALU R r i],
    [AlAI], [Input first argument to calculation], [R],
    [AlBI], [Input second argument to calculation], [r],
    [AlCI], [Input third argument to calculation], [i],
  )
) <microcodes_alu>
#figure(
  caption: "Memory and Register IN Microcodes",
  table(
    align: (left, left, center),
    columns: (0.2fr, 1fr, 0.4fr),
    [Name],    [Description], [Correspondence],
    [DstI],    [Destination register in, full width], [R],
    [DstILLW], [Load into 0-15 bits of destination],  [R],
    [DstILHW], [Load into 16-31 bits of destination], [R],
    [DstIHLW], [Load into 32-47 bits of destination], [R],
    [DstIHHW], [Load into 48-63 bits of destination], [R],
    [], [], [],
    [MemO],    [Load 64 bits from memory], [(64)Mem[r+i]],
    [MemOB],   [Load 8 bits from memory],  [(8) Mem[r+i]],
    [MemOW],   [Load 16 bits from memory], [(16)Mem[r+i]],
    [MemOQ],   [Load 32 bits from memory], [(32)Mem[r+i]],
    [], [], [],
    [MemI],    [Input 64 bits into memory], [(64)Mem[r+i]],
    [MemIB],   [Input 8 bits into memory],  [(8) Mem[r+i]],
    [MemIW],   [Input 16 bits into memory], [(16)Mem[r+i]],
    [MemIQ],   [Input 32 bits into memory], [(32)Mem[r+i]],
  )
) <microcodes_mem_in>
*/
#pagebreak()

#section[Architecture Overview]
Turnstile has 31 general purpose registers and 4 specialized forms:
- r0, the zero register is always set to 0.
- r1-r31 are general purpose registers, they may be used for anything.
- sp is the stack pointer, 
  it may contain any data, 
  but it is used implicitly by some instructions.
  This register is set to the stack top at the start of execution.
- ip is the instruction pointer register. It's the current programs position in code.
- mp is the memory pointer register. It stores an address for memory accesses.

#section[Memory]
#subsection[The Stack]
The stack is a part of the memory that is used to store
local variables and return addresses.
The stack can be located using the stack pointer,
it grows from the end of the address space to the start.

#pagebreak()
#section(skip: false)[Instructions Detailed]
One aspect common to all instructions is the instruction pointer increment, 
it is done before a given instruction finishes executing 
and is important for instructions such as 
#ref_section[Jump, JIfE, JINE, JIfG, JIGE, JIfL, JILE],
as they directly deal with the instruction pointer.

#subsection[Halt and NoOp]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [Halt], [L],    [None], [00000000], [00000000], [00000000], [00000000],
  [NoOp], [L],    [None], [00000000], [00000010], [00000000], [00000000],
))
#part[Description]
Halt~ stops execution.\
NoOp~ does nothing for a cycle.

#instruction_section[LdSg, LdDb, LdQd, LdOc]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [LdSg], [L],    [i],    [00000001], [RRRRR000], [iiiiiiii], [iiiiiiii],
  [LdDb], [L],    [i],    [00000001], [RRRRR010], [iiiiiiii], [iiiiiiii],
  [LdQd], [L],    [i],    [00000001], [RRRRR100], [iiiiiiii], [iiiiiiii],
  [LdOc], [L],    [i],    [00000001], [RRRRR110], [iiiiiiii], [iiiiiiii],
))
#part[Description]
LdSg (Load Single) loads the immediate into the lowest quarter of the register:
#code(
"R &= 0xFFFF FFFF FFFF 0000; 
R |= i;"
)
LdDb (Load Double) loads the immediate into the second-lowest quarter of the register:
#code(
"R &= 0xFFFF FFFF 0000 FFFF;
R |= i << 16;"
)
LdQd (Load Quartic) loads the immediate into the second-highest quarter of the register:
#code(
"R &= 0xFFFF 0000 FFFF FFFF; 
R |= i << 32;"
)
LdOc (Load Octic) loads the immediate into the highest quarter of the register:
#code(
"R &= 0x0000 FFFF FFFF FFFF;
R |= i << 48;"
)

#instruction_section[SxSg, SxDb, SxQd]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [SxSg], [L],    [R],    [00000010], [RRRRR000], [00000000], [00000000],
  [SxDb], [L],    [R],    [00000010], [RRRRR010], [00000000], [00000000],
  [SxQd], [L],    [R],    [00000010], [RRRRR100], [00000000], [00000000],
))
#part[Description]
SxSg~ sign extends the destination register's value based on the 16th bit.
#code("if (R & (1 << 15)) R |= 0xFFFFFFFFFFFF0000;")
SxDb~ sign extends the destination register's value based on the 32nd bit.
#code("if (R & (1 << 31)) R |= 0xFFFFFFFF00000000;")
SxQd~ sign extends the destination register's value based on the 48th bit.
#code("if (R & (1 << 47)) R |= 0xFFFF000000000000;")

#instruction_section[Intr]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [Intr], [L],    [R],    [00000011], [RRRRR000], [00000000], [00000000],
))
#part[Description]
Intr~ calls an interrupt, the code is fetched from the specified register.
Then an interrupt handler table is indexed using said code.
The current instruction pointer is then saved on the stack and a jump to
the handler address is performed.
As of now, as the architecture itself isn't planned to produce interrupts, 
nor receive them from an outside source, calling an interrupt without defining
the handler table before will cause a no-op. As such it is currently only useful
for defining syscalls as an OS or an implementor VM.

#instruction_section[LIHT]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [LIHT], [L],    [R],    [00000100], [RRRRR000], [00000000], [00000000],
))
#part[Description]
LIHT~ loads an interrupt handler table from the address contained in the
specified register. The table is 128 entries long, each entry containing
a 64 bit address to a procedure handling an interrupt.
Each procedure is responsible for backing up and restoring any state
that it might want to leave untouched.


#instruction_section[Dupe]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [Dupe], [T],    [R, r], [00100000], [RRRRR00r], [rrrr0000], [00000000],
))
#part[Description]
Duplicates the contents of the source register into the destination register.
#code("R = r")

#instruction_section[bAnd, bOr, bXor, bNot]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [bAnd], [T],    [R, r], [00100001], [RRRRR00r], [rrrr0000], [00000000],
  [bOr],  [T],    [R, r], [00100001], [RRRRR01r], [rrrr0000], [00000000],
  [bXor], [T],    [R, r], [00100001], [RRRRR10r], [rrrr0000], [00000000],
  [bNot], [T],    [R, r], [00100001], [RRRRR11r], [rrrr0000], [00000000],
))
#part[Description]
bAnd~ sets the destination register to the bitwise AND of the destination and source register values.
#code("R = R & r")
bOr~ sets the destinaiton register to the bitwise OR of the destination and source register values.
#code("R = R | r")
bXor~ sets the destination register to the bitwise XOR of the destination and source register values.
#code("R = R ^ r")
bNot~ sets the destination register to the bitwise NOT of the source register value
#code("R = ~r")

#instruction_section[ShL, ShRZ, ShRO]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [ShL],  [T],    [R, r, i], [00100010], [RRRRR00r], [rrrriiii], [iiiiiiii],
  [ShRZ], [T],    [R, r, i], [00100010], [RRRRR01r], [rrrriiii], [iiiiiiii],
  [ShRO], [T],    [R, r, i], [00100010], [RRRRR11r], [rrrriiii], [iiiiiiii],
))
#part[Description]
ShL~ sets the destination to the destination value shifted left 
by the sum of the source register and immediate value.
The immediate is treated as signed.
#code("R = R << (r + i)")
ShRZ~ sets the destination to the destination value shifted right
by the sum of the source register and immediate value. 
The new incoming bits being zero.
The immediate is treated as signed.
#code("R = R >> (r + i)")
ShRO~ sets the destination to the destination value shifted right
by the sum of the source register and immediate value. 
The new incoming bits being one.
The immediate is treated as signed.
#code("R = R >> (r + i)")

#instruction_section[RdSg, RdDb, RdQd, RdOc]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [RdSg], [T],    [R, r, i], [00100011], [RRRRR00r], [rrrriiii], [iiiiiiii],
  [RdDb], [T],    [R, r, i], [00100011], [RRRRR01r], [rrrriiii], [iiiiiiii],
  [RdQd], [T],    [R, r, i], [00100011], [RRRRR10r], [rrrriiii], [iiiiiiii],
  [RdOc], [T],    [R, r, i], [00100011], [RRRRR11r], [rrrriiii], [iiiiiiii],
))
#part[Description]
RdSg~ reads one byte from memory at the address contained in the source register 
with the immediate as an offset
and puts it into the destination register.
The immediate is treated as signed.
#code("R = (8) [r + i]")
RdDb~ reads two bytes from memory at the address contained in the source register 
with the immediate as an offset
and puts them into the destination register.
The immediate is treated as signed.
#code("R = (16)[r + i]")
RdQd~ reads four bytes from memory at the address contained in the source register 
with the immediate as an offset
and puts them into the destination register.
The immediate is treated as signed.
#code("R = (32)[r + i]")
RdOc~ reads eight bytes from memory at the address contained in the source register 
with the immediate as an offset
and puts them into the destination register.
The immediate is treated as signed.
#code("R = (64)[r + i]")

#instruction_section[StSg, StDb, StQd, StOc]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [StSg], [T],    [R, r, i], [00100100], [RRRRR00r], [rrrriiii], [iiiiiiii],
  [StDb], [T],    [R, r, i], [00100100], [RRRRR01r], [rrrriiii], [iiiiiiii],
  [StQd], [T],    [R, r, i], [00100100], [RRRRR10r], [rrrriiii], [iiiiiiii],
  [StOc], [T],    [R, r, i], [00100100], [RRRRR11r], [rrrriiii], [iiiiiiii],
))
#part[Description]
StSg~ writes one byte from the source register 
into memory at the address contained in the destination register 
with the immediate as an offset.
The immediate is treated as signed.
#code("(8) [R + i] = r")
StDb~ writes two bytes from the source register 
into memory at the address contained in the destination register 
with the immediate as an offset.
The immediate is treated as signed.
#code("(16)[R + i] = r")
StQd~ writes four bytes from the source register 
into memory at the address contained in the destination register 
with the immediate as an offset.
The immediate is treated as signed.
#code("(32)[R + i] = r")
StOc~ writes eight bytes from the source register 
into memory at the address contained in the destination register 
with the immediate as an offset.
The immediate is treated as signed.
#code("(64)[R + i] = r")

#instruction_section[Add, Sub, Mul, Div]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [Add],  [T],    [R, r, i], [00100101], [RRRRR00r], [rrrriiii], [iiiiiiii],
  [Sub],  [T],    [R, r, i], [00100101], [RRRRR01r], [rrrriiii], [iiiiiiii],
  [Mul],  [T],    [R, r, i], [00100101], [RRRRR10r], [rrrriiii], [iiiiiiii],
  [Div],  [T],    [R, r, i], [00100101], [RRRRR11r], [rrrriiii], [iiiiiiii],
))
#part[Description]
Add~ puts the sum of the destination and source registers as well as the immediate 
into the destination register.
#code("R = R + r + i")
Sub~ puts the difference of the destination and source registers as well as the immediate 
into the destination register.
#code("R = R - r - i")
Mul~ puts the product of the destination register and the sum of the source register and the immediate 
into the destination register.
The immediate is treated as signed.
#code("R = R * (r + i)")
Mul~ puts the quotient of the destination register and the sum of the source register and the immediate 
into the destination register.
The immediate is treated as signed.
#code("R = R / (r + i)")

#instruction_section[Jump, JIfE, JINE, JIfG, JIGE, JIfL, JILE]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [Jump], [C],    [R, r, s, a], [01000000], [00000000], [00000000], [000sssss],
  [JIfE], [C],    [R, r, s, a], [01000001], [RRRRR00r], [rrrraaaa], [a00sssss],
  [JINE], [C],    [R, r, s, a], [01000001], [RRRRR01r], [rrrraaaa], [a00sssss], 
  [JIfG], [C],    [R, r, s, a], [01000010], [RRRRR00r], [rrrraaaa], [a00sssss], 
  [JIGE], [C],    [R, r, s, a], [01000010], [RRRRR01r], [rrrraaaa], [a00sssss], 
  [JIfL], [C],    [R, r, s, a], [01000011], [RRRRR00r], [rrrraaaa], [a00sssss], 
  [JILE], [C],    [R, r, s, a], [01000011], [RRRRR01r], [rrrraaaa], [a00sssss],
))
#part[Description]
Jump~ jumps to the address in the additional source register
and place the return address in the return register.
#code("a = ip; ip = s;")
JIfE~ jumps to the address in the additional source register 
if the destination register is equal to the source register
and place the return address in the return register.
#code("if (R == r) {
  a = ip;
  ip = s;
}")
JINE~ jumps to the address in the additional source register 
if the destination register is not equal to the source register
and place the return address in the return register.
#code("if (R != r) {
  a = ip;
  ip = s;
}")
JIfG~ jumps to the address in the additional source register 
if the destination register is greater than the source register
and place the return address in the return register.
#code("if (R > r) {
  a = ip;
  ip = s;
}")
JIGE~ jumps to the address in the additional source register 
if the destination register is greater than or equal to the source register
and place the return address in the return register.
#code("if (R >= r) {
  a = ip;
  ip = s;
}")
JIfL~ jumps to the address in the additional source register 
if the destination register is less than the source register
and place the return address in the return register.
#code("if (R < r) {
  a = ip;
  ip = s;
}")
JILE~ jumps to the address in the additional source register 
if the destination register is less than or equal to the source register
and place the return address in the return register.
#code("if (R <= r) {
  a = ip;
  ip = s;
}")

#instruction_section[Put]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [Put ], [V],    [i],    [01100100], [iiiiiiii], [iiiiiiii], [iiiiiiii],
))
#part[Description]
Put~ prints the c-style string at address in ~r1~ until it either ends 
or the printed length is equal to the immediate.
#code("printf(\"%.*s\", i, r1)")

#instruction_section[PReg]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [PReg], [V],    [i],    [01100101], [00000000], [00000000], [000iiiii],
))
#part[Description]
PReg~ prints the register denoted by the lowest 5 bits of the instruction.
#code("printf(\"r%i: %i\\n\", i & 0b11111, regs[i & 0b11111]);")

#instruction_section[LdFl]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [LdFl], [V],    [i],    [01100110], [000iiiii], [000iiiii], [000iiiii],
))
#part[Description]
Reads a file into memory; the last 5 bits of the 2nd byte denote the register 
containing the destination's address, the last 5 bits of the 3rd byte denote
the register containing the maximum amount of bytes to read and the last 5 bits
of the 4th byte denote the register containing the address of the path.
The path needs to be null-terminated;
A length of 0 reads the entire file.

#instruction_section[WrFl]
#part[Encoding]
#align(center,
table(
  align: (left, center, left, right, right, right, right),
  columns: (4fr, 4fr, 8fr, 8fr, 8fr, 8fr, 8fr), 
  [Name], [Type], [Args], [1st Byte], [2nd Byte], [3rd Byte], [4th Byte],
  [WrFl], [V],    [i],    [01100110], [000iiiii], [000iiiii], [000iiiii],
))
#part[Description]
Writes memory into a file; the last 5 bits of the 2nd byte denote the register 
containing the file path's address, the last 5 bits of the 3rd byte denote
the register containing the maximum amount of bytes to write and the last 5 bits
of the 4th byte denote the register containing the address of the contents to write.
The path needs to be null-terminated;
A length of 0 reads the entire file.

#pagebreak()
#section[Turnstile's ABI]
Before calling a function the caller is responsible for saving their
register states.
Upon entry of a subroutine the callee needs to save the return address, 
which the caller has to put into the first register.
Arguments are passed by registers, 
they are assigned starting from the second register, going up.
Arguments bigger than the register have to be passed by pointer, 
this includes structs, tuples and any construct, 
so long as the size is bigger than the register.
Functions returning types bigger than a register have to allocate
the space on the stack before pushing the return address,
this space will not be deallocated by the function returning.

#section[Further Work]
This section enumerates yet to be specified details.
- Turnstile ABI
- SIMD-extension (wide instruction encodings?)
- Floating Point extensions
#section(level: 1)[Index of Tables]
#outline(
  title: none,
  target: figure.where(kind: table),
)
