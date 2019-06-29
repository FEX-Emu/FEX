# FEXCore Frontend
---
The FEXCore frontend's job is to translate an incoming x86-64 instruction stream in to a more easily digested version of x86.
This effectively expands x86-64 instruction encodings to be more easily ingested later on in the process.
This ends up being essential to allowing our IR translation step to be less strenious. It can decode a "common" expanded instruction format rather than various things that x86-supports.
For a simple example, x86-64's primary op table has ALU ops that duplicate themselves at least six times with minor differences between each. The frontend is able to decode a large amount of these ops to the "same" op that the IR translation understands more readily.
This works for most instructions that follow a common decoding scheme, although there are instructions that don't follow the rules and must be handled explicitly elsewhere.

An example of decoded instructions:
```
00 C0: add al,al
04 01: add al, 0x1
```
These two instructions have a different encoding scheme but they are just an add.
They end up decoding to a generic format with the same destination operand but different sources.
May look subtle but there end up being far more complex cases and we don't want to handle hundreds of instructions differently.
After the frontend is done decoding the instruction stream, it passes the output over to the OpDispatcher for translating to our IR.

## Multiblock
---
The Frontend has an additional duty. Since it is the main piece of code that understands the guest x86-64 code; It is also what does analysis of control flow to determine if we can end up compiling multiple blocks of guest code.
The Frontend already has to determine if it has hit a block ending instruction. This is anything that changes control flow. This feeds in to the analysis system to look at conditional branches to see if we can keep compiling code at the target location in the same functional unit.

Short example:
```
test eax, eax
jne .Continue
ret           <--- We can continue past this instruction, which is an unconditional block ender
.Continue:
```

These sorts of patterns crop up extensively in compiled code. A large amount of traditional JITs will end up ending the block at any sort of conditional branch instruction.
If the analysis can determine the target conditional branch location, we can then know that the code can keep compiling past an unconditional block ender instruction.
This works for both backwards branches and forward branches.

### Additional reading
---
There are other emulators out there that implement multiblock JIT compilation with some success.
The best example of this that I know of is the [Dolphin GameCube and Wii Emulator](https://github.com/dolphin-emu/dolphin) Where I implemented the initial multiblock implementation.
One of the major limitations with a console emulator is that you can run in to infinite loops on backedges when using multiblock compilation. This is due to console emulation being able to run an infinite loop and let Interrupts or some other state cause it to break out.
Luckily since we are a userspace emulator we don't have to deal with this problem. If an application has written an infinite loop, then without another thread running, it'll be a true infinite loop.
Additionally luckily is that we are going to emulate the strong memory model of x86-64 and also support true threads, this will mean that we don't need to do any manual thread scheduling in our emulator and switch between virtual threads.

