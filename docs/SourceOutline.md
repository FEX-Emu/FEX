# FEX-2405

## FEXCore
See [FEXCore/Readme.md](../FEXCore/Readme.md) for more details

### Glossary

- Splatter: a code generator backend that concaternates configurable macros instead of doing isel
- IR: Intermediate Representation, our high-level opcode representation, loosely modeling arm64
- SSA: Single Static Assignment, a form of representing IR in memory
- Basic Block: A block of instructions with no control flow, terminated by control flow
- Fragment: A Collection of basic blocks, possibly an entire guest function or a subset of it


### backend
IR to host code generation

#### arm64
- [ALUOps.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/ALUOps.cpp)
- [Arm64Relocations.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/Arm64Relocations.cpp): relocation logic of the arm64 splatter backend
- [AtomicOps.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/AtomicOps.cpp)
- [BranchOps.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/BranchOps.cpp)
- [ConversionOps.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/ConversionOps.cpp)
- [EncryptionOps.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/EncryptionOps.cpp)
- [FlagOps.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/FlagOps.cpp)
- [JIT.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/JIT.cpp): Main glue logic of the arm64 splatter backend
- [JITClass.h](../FEXCore/Source/Interface/Core/JIT/Arm64/JITClass.h)
- [MemoryOps.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/MemoryOps.cpp)
- [MiscOps.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/MiscOps.cpp)
- [MoveOps.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/MoveOps.cpp)
- [VectorOps.cpp](../FEXCore/Source/Interface/Core/JIT/Arm64/VectorOps.cpp)

#### shared
- [CPUBackend.h](../FEXCore/Source/Interface/Core/CPUBackend.h)



### frontend

#### x86-meta-blocks
- [Frontend.cpp](../FEXCore/Source/Interface/Core/Frontend.cpp): Extracts instruction & block meta info, frontend multiblock logic

#### x86-tables
Metadata that drives the frontend x86/64 decoding
- [BaseTables.cpp](../FEXCore/Source/Interface/Core/X86Tables/BaseTables.cpp)
- [DDDTables.cpp](../FEXCore/Source/Interface/Core/X86Tables/DDDTables.cpp)
- [EVEXTables.cpp](../FEXCore/Source/Interface/Core/X86Tables/EVEXTables.cpp)
- [H0F38Tables.cpp](../FEXCore/Source/Interface/Core/X86Tables/H0F38Tables.cpp)
- [H0F3ATables.cpp](../FEXCore/Source/Interface/Core/X86Tables/H0F3ATables.cpp)
- [PrimaryGroupTables.cpp](../FEXCore/Source/Interface/Core/X86Tables/PrimaryGroupTables.cpp)
- [SecondaryGroupTables.cpp](../FEXCore/Source/Interface/Core/X86Tables/SecondaryGroupTables.cpp)
- [SecondaryModRMTables.cpp](../FEXCore/Source/Interface/Core/X86Tables/SecondaryModRMTables.cpp)
- [SecondaryTables.cpp](../FEXCore/Source/Interface/Core/X86Tables/SecondaryTables.cpp)
- [VEXTables.cpp](../FEXCore/Source/Interface/Core/X86Tables/VEXTables.cpp)
- [X86Tables.h](../FEXCore/Source/Interface/Core/X86Tables/X86Tables.h)
- [X87Tables.cpp](../FEXCore/Source/Interface/Core/X86Tables/X87Tables.cpp)
- [XOPTables.cpp](../FEXCore/Source/Interface/Core/X86Tables/XOPTables.cpp)
- [X86Tables.cpp](../FEXCore/Source/Interface/Core/X86Tables.cpp)

#### x86-to-ir
- [Crypto.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher/Crypto.cpp): Handles x86/64 Crypto instructions to IR
- [Flags.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher/Flags.cpp): Handles x86/64 flag generation
- [Vector.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher/Vector.cpp): Handles x86/64 Vector instructions to IR
- [X87.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher/X87.cpp): Handles x86/64 x87 to IR
- [X87F64.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher/X87F64.cpp): Handles x86/64 x87 to IR
- [OpcodeDispatcher.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher.cpp): Handles x86/64 ops to IR, no-pf opt, local-flags opt



### glue
Logic that binds various parts together

#### block-database
- [LookupCache.cpp](../FEXCore/Source/Interface/Core/LookupCache.cpp): Stores information about blocks, and provides C++ implementations to lookup the blocks

#### driver
Emulation mainloop related glue logic
- [Core.cpp](../FEXCore/Source/Interface/Core/Core.cpp): Glues Frontend, OpDispatcher and IR Opts & Compilation, LookupCache, Dispatcher and provides the Execution loop entrypoint

#### log-manager
- [LogManager.cpp](../FEXCore/Source/Utils/LogManager.cpp)

#### thunks
FEXCore side of thunks: Registration, Lookup
- [Thunks.cpp](../FEXCore/Source/Interface/HLE/Thunks/Thunks.cpp)
- [Thunks.h](../FEXCore/Source/Interface/HLE/Thunks/Thunks.h)

#### x86-guest-code
- [X86HelperGen.cpp](../FEXCore/Source/Interface/Core/X86HelperGen.cpp): Guest-side assembly helpers used by the backends
- [X86HelperGen.h](../FEXCore/Source/Interface/Core/X86HelperGen.h)



### ir

#### debug
- [IRDumperPass.cpp](../FEXCore/Source/Interface/IR/Passes/IRDumperPass.cpp): Prints IR

#### dumper
IR -> Text
- [IRDumper.cpp](../FEXCore/Source/Interface/IR/IRDumper.cpp)

#### emitter
C++ Functions to generate IR. See IR.json for spec.
- [IREmitter.cpp](../FEXCore/Source/Interface/IR/IREmitter.cpp)

#### opts
IR to IR Optimization
- [PassManager.cpp](../FEXCore/Source/Interface/IR/PassManager.cpp): Defines which passes are run, and runs them
- [PassManager.h](../FEXCore/Source/Interface/IR/PassManager.h)
- [ConstProp.cpp](../FEXCore/Source/Interface/IR/Passes/ConstProp.cpp): ConstProp, ZExt elim, addressgen coalesce, const pooling, fcmp reduction, const inlining
- [DeadContextStoreElimination.cpp](../FEXCore/Source/Interface/IR/Passes/DeadContextStoreElimination.cpp): Transforms ContextLoad/Store to temporaries, similar to mem2reg
- [DeadStoreElimination.cpp](../FEXCore/Source/Interface/IR/Passes/DeadStoreElimination.cpp): Cross block store-after-store elimination
- [IRValidation.cpp](../FEXCore/Source/Interface/IR/Passes/IRValidation.cpp): Sanity checking pass
- [InlineCallOptimization.cpp](../FEXCore/Source/Interface/IR/Passes/InlineCallOptimization.cpp): Removes unused arguments if known syscall number
- [LongDivideRemovalPass.cpp](../FEXCore/Source/Interface/IR/Passes/LongDivideRemovalPass.cpp): Long divide elimination pass
- [RedundantFlagCalculationElimination.cpp](../FEXCore/Source/Interface/IR/Passes/RedundantFlagCalculationElimination.cpp): This is not used right now, possibly broken
- [RegisterAllocationPass.cpp](../FEXCore/Source/Interface/IR/Passes/RegisterAllocationPass.cpp)
- [RegisterAllocationPass.h](../FEXCore/Source/Interface/IR/Passes/RegisterAllocationPass.h)

### opcodes

#### cpuid
- [CPUID.cpp](../FEXCore/Source/Interface/Core/CPUID.cpp): Handles presented capability bits for guest cpu

#### dispatcher-implementations
- [Crypto.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher/Crypto.cpp): Handles x86/64 Crypto instructions to IR
- [Flags.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher/Flags.cpp): Handles x86/64 flag generation
- [Vector.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher/Vector.cpp): Handles x86/64 Vector instructions to IR
- [X87.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher/X87.cpp): Handles x86/64 x87 to IR
- [X87F64.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher/X87F64.cpp): Handles x86/64 x87 to IR
- [OpcodeDispatcher.cpp](../FEXCore/Source/Interface/Core/OpcodeDispatcher.cpp): Handles x86/64 ops to IR, no-pf opt, local-flags opt

## ThunkLibs
See [ThunkLibs/README.md](../ThunkLibs/README.md) for more details

### thunklibs
These are generated + glue logic 1:1 thunks unless noted otherwise

#### EGL
- [libEGL_Guest.cpp](../ThunkLibs/libEGL/libEGL_Guest.cpp): Depends on glXGetProcAddress thunk
- [libEGL_Host.cpp](../ThunkLibs/libEGL/libEGL_Host.cpp)

#### GL
- [libGL_Guest.cpp](../ThunkLibs/libGL/libGL_Guest.cpp): Handles glXGetProcAddress
- [libGL_Host.cpp](../ThunkLibs/libGL/libGL_Host.cpp): Uses glXGetProcAddress instead of dlsym

#### SDL2
- [libSDL2_Guest.cpp](../ThunkLibs/libSDL2/libSDL2_Guest.cpp): Handles sdlglproc, dload, stubs a few log fns
- [libSDL2_Host.cpp](../ThunkLibs/libSDL2/libSDL2_Host.cpp)

#### VDSO
- [libVDSO_Guest.cpp](../ThunkLibs/libVDSO/libVDSO_Guest.cpp): Linux VDSO thunking

#### Vulkan
- [Guest.cpp](../ThunkLibs/libvulkan/Guest.cpp)
- [Host.cpp](../ThunkLibs/libvulkan/Host.cpp)

#### X11
- [libXext_Guest.cpp](../ThunkLibs/libXext/libXext_Guest.cpp)
- [libXext_Host.cpp](../ThunkLibs/libXext/libXext_Host.cpp)
- [libXfixes_Guest.cpp](../ThunkLibs/libXfixes/libXfixes_Guest.cpp)
- [libXfixes_Host.cpp](../ThunkLibs/libXfixes/libXfixes_Host.cpp)
- [libXrender_Guest.cpp](../ThunkLibs/libXrender/libXrender_Guest.cpp)
- [libXrender_Host.cpp](../ThunkLibs/libXrender/libXrender_Host.cpp)

#### asound
- [libasound_Guest.cpp](../ThunkLibs/libasound/libasound_Guest.cpp)
- [libasound_Host.cpp](../ThunkLibs/libasound/libasound_Host.cpp)

#### cef
- [libcef_Guest.cpp](../ThunkLibs/libcef/libcef_Guest.cpp)
- [libcef_Host.cpp](../ThunkLibs/libcef/libcef_Host.cpp)

#### drm
- [Guest.cpp](../ThunkLibs/libdrm/Guest.cpp)
- [Host.cpp](../ThunkLibs/libdrm/Host.cpp)

#### fex_malloc
- [Guest.cpp](../ThunkLibs/libfex_malloc/Guest.cpp): Handles allocations between guest and host thunks
- [Host.cpp](../ThunkLibs/libfex_malloc/Host.cpp): Handles allocations between guest and host thunks

#### fex_malloc_loader
- [Guest.cpp](../ThunkLibs/libfex_malloc_loader/Guest.cpp): Delays malloc symbol replacement until it is safe to run constructors

#### fex_malloc_symbols
- [Host.cpp](../ThunkLibs/libfex_malloc_symbols/Host.cpp): Allows FEX to export allocation symbols

#### fex_thunk_test
- [Guest.cpp](../ThunkLibs/libfex_thunk_test/Guest.cpp)
- [Host.cpp](../ThunkLibs/libfex_thunk_test/Host.cpp)

#### wayland-client
- [Guest.cpp](../ThunkLibs/libwayland-client/Guest.cpp)
- [Host.cpp](../ThunkLibs/libwayland-client/Host.cpp)

#### xshmfence
- [Guest.cpp](../ThunkLibs/libxshmfence/Guest.cpp)
- [Host.cpp](../ThunkLibs/libxshmfence/Host.cpp)

## Source/Tests

## unittests
See [unittests/Readme.md](../unittests/Readme.md) for more details

