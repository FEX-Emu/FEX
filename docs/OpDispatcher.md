# FEXCore OpDispatcher
---
The OpDispatcher is the step of the recompiler that takes the output from the Frontend and translates it to our IR.
Since the x86-64 instruction set is so large (>1000 instructions in the current FEXCore tables) we need to reduce this down to something more manageable.
We will ingest our decoded x86-64 instructions and translate them down to more basic IR operations. The number of IR ops are currently in the dozens which is a lot easier to handle.
Once we have translated to the IR then we need to pass the IR over to optimization passes or our JIT cores.

Ex:
```
 mov rax,0x1
 mov rdi,0x1
 mov rsi,0x20
 mov rdx,0x1
 syscall 
 hlt
 ```
 Translates to the IR of:
 ```
BeginBlock
        %ssa8 i32 = Constant 0x1
        StoreContext 0x8, 0x8, %ssa8
        %ssa64 i32 = Constant 0x1
        StoreContext 0x8, 0x30, %ssa64
        %ssa120 i32 = Constant 0x1f
        StoreContext 0x8, 0x28, %ssa120
        %ssa176 i32 = Constant 0x1
        StoreContext 0x8, 0x20, %ssa176
        %ssa232 i64 = LoadContext 0x8, 0x8
        %ssa264 i64 = LoadContext 0x8, 0x30
        %ssa296 i64 = LoadContext 0x8, 0x28
        %ssa328 i64 = LoadContext 0x8, 0x20
        %ssa360 i64 = LoadContext 0x8, 0x58
        %ssa392 i64 = LoadContext 0x8, 0x48
        %ssa424 i64 = LoadContext 0x8, 0x50
        %ssa456 i64 = Syscall%ssa232, %ssa264, %ssa296, %ssa328, %ssa360, %ssa392, %ssa424
        StoreContext 0x8, 0x8, %ssa456
        BeginBlock
        EndBlock 0x1e
        ExitFunction
```
### Multiblock
---
An additional duty of the OpDispatcher is to handle the metadata that the Frontend provides for supporting multiblock.
The IR provides most of the functionality required for supporting robust branching and function creation required for generating large blocks of code translated from x86-64 emulation.
This is required since in the ideal situation we will be doing function level translation of x86-64 guest code to our IR.
The IR is currently lacking any idea of flags or PHI nodes, which can be problematic when optimizing branch heavy code. The good thing is that the LLVM JIT can use a mem to reg pass to minimize a large number of this code.
It **will** be required to improve the IR further once the runtime JIT becomes a higher priority
