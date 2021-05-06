# FEX-2105

## External/FEXCore
See [FEXCore/Readme.md](../External/FEXCore/Readme.md) for more details

### Glossary

- Splatter: a code generator backend that concaternates configurable macros instead of doing isel
- IR: Intermediate Representation, our high-level opcode representation, loosely modeling arm64
- SSA: Single Static Assignment, a form of representing IR in memory
- Basic Block: A block of instructions with no control flow, terminated by control flow
- Fragment: A Collection of basic blocks, possibly an entire guest function or a subset of it


### backend
IR to host code generation

#### arm64
- [ALUOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/Arm64/ALUOps.cpp)
- [AtomicOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/Arm64/AtomicOps.cpp)
- [BranchOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/Arm64/BranchOps.cpp)
- [ConversionOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/Arm64/ConversionOps.cpp)
- [EncryptionOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/Arm64/EncryptionOps.cpp)
- [FlagOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/Arm64/FlagOps.cpp)
- [JIT.cpp](../External/FEXCore/Source/Interface/Core/JIT/Arm64/JIT.cpp): Main glue logic of the arm64 splatter backend
- [JITClass.h](../External/FEXCore/Source/Interface/Core/JIT/Arm64/JITClass.h)
- [MemoryOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/Arm64/MemoryOps.cpp)
- [MiscOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/Arm64/MiscOps.cpp)
- [MoveOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/Arm64/MoveOps.cpp)
- [VectorOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/Arm64/VectorOps.cpp)

#### shared
- [CPUBackend.h](../External/FEXCore/include/FEXCore/Core/CPUBackend.h)

#### x86-64
- [ALUOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/x86_64/ALUOps.cpp)
- [AtomicOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/x86_64/AtomicOps.cpp)
- [BranchOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/x86_64/BranchOps.cpp)
- [ConversionOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/x86_64/ConversionOps.cpp)
- [EncryptionOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/x86_64/EncryptionOps.cpp)
- [FlagOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/x86_64/FlagOps.cpp)
- [JIT.cpp](../External/FEXCore/Source/Interface/Core/JIT/x86_64/JIT.cpp): Main glue logic of the x86-64 splatter backend
- [JITClass.h](../External/FEXCore/Source/Interface/Core/JIT/x86_64/JITClass.h)
- [MemoryOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/x86_64/MemoryOps.cpp)
- [MiscOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/x86_64/MiscOps.cpp)
- [MoveOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/x86_64/MoveOps.cpp)
- [VectorOps.cpp](../External/FEXCore/Source/Interface/Core/JIT/x86_64/VectorOps.cpp)



### frontend

#### x86-meta-blocks
- [Frontend.cpp](../External/FEXCore/Source/Interface/Core/Frontend.cpp): Extracts instruction & block meta info, frontend multiblock logic

#### x86-tables
Metadata that drives the frontend x86/64 decoding
- [BaseTables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/BaseTables.cpp)
- [DDDTables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/DDDTables.cpp)
- [EVEXTables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/EVEXTables.cpp)
- [H0F38Tables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/H0F38Tables.cpp)
- [H0F3ATables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/H0F3ATables.cpp)
- [PrimaryGroupTables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/PrimaryGroupTables.cpp)
- [SecondaryGroupTables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/SecondaryGroupTables.cpp)
- [SecondaryModRMTables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/SecondaryModRMTables.cpp)
- [SecondaryTables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/SecondaryTables.cpp)
- [VEXTables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/VEXTables.cpp)
- [X86Tables.h](../External/FEXCore/Source/Interface/Core/X86Tables/X86Tables.h)
- [X87Tables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/X87Tables.cpp)
- [XOPTables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables/XOPTables.cpp)
- [X86Tables.cpp](../External/FEXCore/Source/Interface/Core/X86Tables.cpp)

#### x86-to-ir
- [OpcodeDispatcher.cpp](../External/FEXCore/Source/Interface/Core/OpcodeDispatcher.cpp): Handles x86/64 ops to IR, no-pf opt, local-flags opt



### glue
Logic that binds various parts together

#### block-database
- [LookupCache.cpp](../External/FEXCore/Source/Interface/Core/LookupCache.cpp): Stores information about blocks, and provides C++ implementations to lookup the blocks

#### driver
Emulation mainloop related glue logic
- [Core.cpp](../External/FEXCore/Source/Interface/Core/Core.cpp): Glues Frontend, OpDispatcher and IR Opts & Compilation, LookupCache, Dispatcher and provides the Execution loop entrypoint

#### elf-parsing
- [ELFContainer.cpp](../External/FEXCore/Source/Utils/ELFContainer.cpp): Loads and parses an elf to memory. Also handles some loading & logic.
- [ELFSymbolDatabase.cpp](../External/FEXCore/Source/Utils/ELFSymbolDatabase.cpp): Part of our now defunct ld-linux replacement, keeps tracks of all symbols, loads elfs, handles relocations. Small parts of this are used.

#### gdbserver
- [GdbServer.cpp](../External/FEXCore/Source/Interface/Core/GdbServer.cpp): Provides a gdb interface to the guest state
- [GdbServer.h](../External/FEXCore/Source/Interface/Core/GdbServer.h)

#### log-manager
- [LogManager.cpp](../External/FEXCore/Source/Utils/LogManager.cpp)

#### thunks
FEXCore side of thunks: Registration, Lookup
- [Thunks.cpp](../External/FEXCore/Source/Interface/HLE/Thunks/Thunks.cpp)
- [Thunks.h](../External/FEXCore/Source/Interface/HLE/Thunks/Thunks.h)

#### x86-guest-code
- [X86HelperGen.cpp](../External/FEXCore/Source/Interface/Core/X86HelperGen.cpp): Guest-side assembly helpers used by the backends
- [X86HelperGen.h](../External/FEXCore/Source/Interface/Core/X86HelperGen.h)



### ir

#### dumper
IR -> Text
- [IRDumper.cpp](../External/FEXCore/Source/Interface/IR/IRDumper.cpp)

#### emitter
C++ Functions to generate IR. See IR.json for spec.
- [IREmitter.cpp](../External/FEXCore/Source/Interface/IR/IREmitter.cpp)

#### opts
IR to IR Optimization
- [PassManager.cpp](../External/FEXCore/Source/Interface/IR/PassManager.cpp): Defines which passes are run, and runs them
- [PassManager.h](../External/FEXCore/Source/Interface/IR/PassManager.h)
- [ConstProp.cpp](../External/FEXCore/Source/Interface/IR/Passes/ConstProp.cpp): ConstProp, ZExt elim, addressgen coalesce, const pooling, fcmp reduction, const inlining
- [DeadCodeElimination.cpp](../External/FEXCore/Source/Interface/IR/Passes/DeadCodeElimination.cpp)
- [DeadContextStoreElimination.cpp](../External/FEXCore/Source/Interface/IR/Passes/DeadContextStoreElimination.cpp): Transforms ContextLoad/Store to temporaries, similar to mem2reg
- [DeadStoreElimination.cpp](../External/FEXCore/Source/Interface/IR/Passes/DeadStoreElimination.cpp): Cross block store-after-store elimination
- [IRCompaction.cpp](../External/FEXCore/Source/Interface/IR/Passes/IRCompaction.cpp): Sorts the ssa storage in memory, needed for RA and others
- [IRValidation.cpp](../External/FEXCore/Source/Interface/IR/Passes/IRValidation.cpp): Sanity checking pass
- [LongDivideRemovalPass.cpp](../External/FEXCore/Source/Interface/IR/Passes/LongDivideRemovalPass.cpp): Long divide elimination pass
- [PhiValidation.cpp](../External/FEXCore/Source/Interface/IR/Passes/PhiValidation.cpp): Sanity checking pass
- [RedundantFlagCalculationElimination.cpp](../External/FEXCore/Source/Interface/IR/Passes/RedundantFlagCalculationElimination.cpp): This is not used right now, possibly broken
- [RegisterAllocationPass.cpp](../External/FEXCore/Source/Interface/IR/Passes/RegisterAllocationPass.cpp)
- [RegisterAllocationPass.h](../External/FEXCore/Source/Interface/IR/Passes/RegisterAllocationPass.h)
- [StaticRegisterAllocationPass.cpp](../External/FEXCore/Source/Interface/IR/Passes/StaticRegisterAllocationPass.cpp): Replaces Load/StoreContext with Load/StoreReg for SRA regs
- [SyscallOptimization.cpp](../External/FEXCore/Source/Interface/IR/Passes/SyscallOptimization.cpp): Removes unused arguments if known syscall number
- [ValueDominanceValidation.cpp](../External/FEXCore/Source/Interface/IR/Passes/ValueDominanceValidation.cpp): Sanity Checking

#### parser
Text -> IR
- [IRParser.cpp](../External/FEXCore/Source/Interface/IR/IRParser.cpp)



### opcodes

#### cpuid
- [CPUID.cpp](../External/FEXCore/Source/Interface/Core/CPUID.cpp): Handles presented capability bits for guest cpu

#### dispatcher-implementations
- [OpcodeDispatcher.cpp](../External/FEXCore/Source/Interface/Core/OpcodeDispatcher.cpp): Handles x86/64 ops to IR, no-pf opt, local-flags opt

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

#### X11
- [libX11_Guest.cpp](../ThunkLibs/libX11/libX11_Guest.cpp): Handles callbacks and varargs
- [libX11_Host.cpp](../ThunkLibs/libX11/libX11_Host.cpp): Handles callbacks and varargs
- [libXext_Guest.cpp](../ThunkLibs/libXext/libXext_Guest.cpp)
- [libXext_Host.cpp](../ThunkLibs/libXext/libXext_Host.cpp)
- [libXfixes_Guest.cpp](../ThunkLibs/libXfixes/libXfixes_Guest.cpp)
- [libXfixes_Host.cpp](../ThunkLibs/libXfixes/libXfixes_Host.cpp)
- [libXrender_Guest.cpp](../ThunkLibs/libXrender/libXrender_Guest.cpp)
- [libXrender_Host.cpp](../ThunkLibs/libXrender/libXrender_Host.cpp)

#### asound
- [libasound_Guest.cpp](../ThunkLibs/libasound/libasound_Guest.cpp)
- [libasound_Host.cpp](../ThunkLibs/libasound/libasound_Host.cpp)

## Source/Tests

### Bin

#### FEXBash
- [FEXBash.cpp](../Source/Tests/FEXBash.cpp): Launches bash under FEX and passes arguments via -c to it

#### FEXLoader
- [FEXLoader.cpp](../Source/Tests/FEXLoader.cpp): Glues the ELF loader, FEXCore and LinuxSyscalls to launch an elf under fex

#### IRLoader
- [IRLoader.cpp](../Source/Tests/IRLoader.cpp): Used to run IR Tests

#### TestHarnessRunner
- [TestHarnessRunner.cpp](../Source/Tests/TestHarnessRunner.cpp): Used to run Assembly tests

#### UnitTestGenerator
- [UnitTestGenerator.cpp](../Source/Tests/UnitTestGenerator.cpp): Brute forces generation of tests for x86/64, incomplete, unused right now



### LinuxSyscalls
Linux syscall emulation, marshaling and passthrough

#### common
- [EmulatedFiles.cpp](../Source/Tests/LinuxSyscalls/EmulatedFiles/EmulatedFiles.cpp): Emulated /proc/cpuinfo, version, osrelease, etc
- [EmulatedFiles.h](../Source/Tests/LinuxSyscalls/EmulatedFiles/EmulatedFiles.h)
- [FileManagement.cpp](../Source/Tests/LinuxSyscalls/FileManagement.cpp): Rootfs overlay logic
- [FileManagement.h](../Source/Tests/LinuxSyscalls/FileManagement.h)
- [SignalDelegator.cpp](../Source/Tests/LinuxSyscalls/SignalDelegator.cpp): Handles host -> host and host -> guest signal routing, emulates procmask & co
- [SignalDelegator.h](../Source/Tests/LinuxSyscalls/SignalDelegator.h)
- [Syscalls.cpp](../Source/Tests/LinuxSyscalls/Syscalls.cpp): Glue logic, brk allocations
- [Syscalls.h](../Source/Tests/LinuxSyscalls/Syscalls.h): Glue logic, STRACE magic

#### syscalls-shared
Syscall implementations shared between x86 and x86-64
- [EPoll.cpp](../Source/Tests/LinuxSyscalls/Syscalls/EPoll.cpp)
- [FD.cpp](../Source/Tests/LinuxSyscalls/Syscalls/FD.cpp)
- [FS.cpp](../Source/Tests/LinuxSyscalls/Syscalls/FS.cpp)
- [IO.cpp](../Source/Tests/LinuxSyscalls/Syscalls/IO.cpp)
- [Info.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Info.cpp)
- [Key.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Key.cpp)
- [Memory.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Memory.cpp)
- [Msg.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Msg.cpp)
- [NotImplemented.cpp](../Source/Tests/LinuxSyscalls/Syscalls/NotImplemented.cpp)
- [SHM.cpp](../Source/Tests/LinuxSyscalls/Syscalls/SHM.cpp)
- [Sched.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Sched.cpp)
- [Semaphore.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Semaphore.cpp)
- [Signals.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Signals.cpp)
- [Socket.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Socket.cpp)
- [Stubs.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Stubs.cpp)
- [Thread.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Thread.cpp)
- [Thread.h](../Source/Tests/LinuxSyscalls/Syscalls/Thread.h)
- [Time.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Time.cpp)
- [Timer.cpp](../Source/Tests/LinuxSyscalls/Syscalls/Timer.cpp)

#### syscalls-x86-32
x86-32 specific syscall implementations
- [EPoll.cpp](../Source/Tests/LinuxSyscalls/x32/EPoll.cpp)
- [FD.cpp](../Source/Tests/LinuxSyscalls/x32/FD.cpp)
- [FS.cpp](../Source/Tests/LinuxSyscalls/x32/FS.cpp)
- [Info.cpp](../Source/Tests/LinuxSyscalls/x32/Info.cpp)
- [Memory.cpp](../Source/Tests/LinuxSyscalls/x32/Memory.cpp)
- [Msg.cpp](../Source/Tests/LinuxSyscalls/x32/Msg.cpp)
- [NotImplemented.cpp](../Source/Tests/LinuxSyscalls/x32/NotImplemented.cpp)
- [Sched.cpp](../Source/Tests/LinuxSyscalls/x32/Sched.cpp)
- [Semaphore.cpp](../Source/Tests/LinuxSyscalls/x32/Semaphore.cpp)
- [Signals.cpp](../Source/Tests/LinuxSyscalls/x32/Signals.cpp)
- [Socket.cpp](../Source/Tests/LinuxSyscalls/x32/Socket.cpp)
- [Syscalls.cpp](../Source/Tests/LinuxSyscalls/x32/Syscalls.cpp)
- [Syscalls.h](../Source/Tests/LinuxSyscalls/x32/Syscalls.h)
- [SyscallsEnum.h](../Source/Tests/LinuxSyscalls/x32/SyscallsEnum.h)
- [Thread.cpp](../Source/Tests/LinuxSyscalls/x32/Thread.cpp)
- [Thread.h](../Source/Tests/LinuxSyscalls/x32/Thread.h)
- [Time.cpp](../Source/Tests/LinuxSyscalls/x32/Time.cpp)
- [Timer.cpp](../Source/Tests/LinuxSyscalls/x32/Timer.cpp)
- [Types.h](../Source/Tests/LinuxSyscalls/x32/Types.h)

#### syscalls-x86-64
x86-64 specific syscall implementations
- [EPoll.cpp](../Source/Tests/LinuxSyscalls/x64/EPoll.cpp)
- [FD.cpp](../Source/Tests/LinuxSyscalls/x64/FD.cpp)
- [IO.cpp](../Source/Tests/LinuxSyscalls/x64/IO.cpp)
- [Info.cpp](../Source/Tests/LinuxSyscalls/x64/Info.cpp)
- [Ioctl.cpp](../Source/Tests/LinuxSyscalls/x64/Ioctl.cpp)
- [Memory.cpp](../Source/Tests/LinuxSyscalls/x64/Memory.cpp)
- [Msg.cpp](../Source/Tests/LinuxSyscalls/x64/Msg.cpp)
- [NotImplemented.cpp](../Source/Tests/LinuxSyscalls/x64/NotImplemented.cpp)
- [Sched.cpp](../Source/Tests/LinuxSyscalls/x64/Sched.cpp)
- [Semaphore.cpp](../Source/Tests/LinuxSyscalls/x64/Semaphore.cpp)
- [Signals.cpp](../Source/Tests/LinuxSyscalls/x64/Signals.cpp)
- [Socket.cpp](../Source/Tests/LinuxSyscalls/x64/Socket.cpp)
- [Syscalls.cpp](../Source/Tests/LinuxSyscalls/x64/Syscalls.cpp)
- [Syscalls.h](../Source/Tests/LinuxSyscalls/x64/Syscalls.h)
- [SyscallsEnum.h](../Source/Tests/LinuxSyscalls/x64/SyscallsEnum.h)
- [Thread.cpp](../Source/Tests/LinuxSyscalls/x64/Thread.cpp)
- [Thread.h](../Source/Tests/LinuxSyscalls/x64/Thread.h)
- [Time.cpp](../Source/Tests/LinuxSyscalls/x64/Time.cpp)

## unittests
See [unittests/Readme.md](../unittests/Readme.md) for more details

