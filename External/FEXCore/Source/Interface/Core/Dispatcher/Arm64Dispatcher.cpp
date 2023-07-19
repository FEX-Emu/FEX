#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/LookupCache.h"

#include "Interface/Core/Dispatcher/Arm64Dispatcher.h"
#include "Interface/Core/Interpreter/InterpreterClass.h"

#include "Interface/Context/Context.h"
#include "Interface/Core/X86HelperGen.h"

#include <FEXCore/Core/CPUBackend.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/fextl/memory.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>

#include <aarch64/assembler-aarch64.h>
#include <aarch64/constants-aarch64.h>
#include <aarch64/cpu-aarch64.h>
#include <aarch64/operands-aarch64.h>
#include <code-buffer-vixl.h>
#include <platform-vixl.h>

#include <unistd.h>

namespace FEXCore::CPU {

constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096;

Arm64Dispatcher::Arm64Dispatcher(FEXCore::Context::ContextImpl *ctx, const DispatcherConfig &config)
  : FEXCore::CPU::Dispatcher(ctx, config), Arm64Emitter(ctx, MAX_DISPATCHER_CODE_SIZE)
#ifdef VIXL_SIMULATOR
  , Simulator {&Decoder}
#endif
{
#ifdef VIXL_SIMULATOR
  // Hardcode a 256-bit vector width if we are running in the simulator.
  Simulator.SetVectorLengthInBits(256);
#endif

  EmitDispatcher();
}

void Arm64Dispatcher::EmitDispatcher() {
#ifdef VIXL_DISASSEMBLER
  const auto DisasmBegin = GetCursorAddress<const vixl::aarch64::Instruction*>();
#endif

  DispatchPtr = GetCursorAddress<AsmDispatch>();

  // while (true) {
  //    Ptr = FindBlock(RIP)
  //    if (!Ptr)
  //      Ptr = CTX->CompileBlock(RIP);
  //
  //    Ptr();
  // }

  ARMEmitter::ForwardLabel l_CTX;
  ARMEmitter::ForwardLabel l_Sleep;
  ARMEmitter::ForwardLabel l_CompileBlock;

  // Push all the register we need to save
  PushCalleeSavedRegisters();

  // Push our memory base to the correct register
  // Move our thread pointer to the correct register
  // This is passed in to parameter 0 (x0)
  mov(STATE, ARMEmitter::XReg::x0);

  // Save this stack pointer so we can cleanly shutdown the emulation with a long jump
  // regardless of where we were in the stack
  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, ARMEmitter::Reg::rsp, 0);
  str(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, ReturningStackLocation));

  AbsoluteLoopTopAddressFillSRA = GetCursorAddress<uint64_t>();

  if (config.StaticRegisterAllocation) {
    FillStaticRegs();
  }

  // We want to ensure that we are 16 byte aligned at the top of this loop
  Align16B();
  ARMEmitter::BiDirectionalLabel FullLookup{};
  ARMEmitter::BiDirectionalLabel CallBlock{};
  ARMEmitter::BackwardLabel LoopTop{};

  Bind(&LoopTop);
  AbsoluteLoopTopAddress = GetCursorAddress<uint64_t>();

  // Load in our RIP
  // Don't modify x2 since it contains our RIP once the block doesn't exist

  auto RipReg = ARMEmitter::XReg::x2;
  ldr(RipReg, STATE_PTR(CpuStateFrame, State.rip));

  // L1 Cache
  ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, Pointers.Common.L1Pointer));

  and_(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r3, RipReg.R(), LookupCache::L1_ENTRIES_MASK);
  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, ARMEmitter::Reg::r0, ARMEmitter::Reg::r3, ARMEmitter::ShiftType::LSL , 4);
  ldp<ARMEmitter::IndexType::OFFSET>(ARMEmitter::XReg::x3, ARMEmitter::XReg::x0, ARMEmitter::Reg::r0, 0);
  cmp(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, RipReg.R());
  b(ARMEmitter::Condition::CC_NE, &FullLookup);

  br(ARMEmitter::Reg::r3);

  // L1C check failed, do a full lookup
  Bind(&FullLookup);

  // This is the block cache lookup routine
  // It matches what is going on it LookupCache.h::FindBlock
  ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, Pointers.Common.L2Pointer));

  // Mask the address by the virtual address size so we can check for aliases
  uint64_t VirtualMemorySize = CTX->Config.VirtualMemSize;
  if (std::popcount(VirtualMemorySize) == 1) {
    and_(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r3, RipReg.R(), VirtualMemorySize - 1);
  }
  else {
    LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r3, VirtualMemorySize);
    and_(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r3, RipReg.R(), ARMEmitter::Reg::r3);
  }

  ARMEmitter::ForwardLabel NoBlock;

  {
    // Offset the address and add to our page pointer
    lsr(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, ARMEmitter::Reg::r3, 12);

    // Load the pointer from the offset
    ldr(ARMEmitter::XReg::x0, ARMEmitter::Reg::r0, ARMEmitter::Reg::r1, ARMEmitter::ExtendedType::LSL_64, 3);

    // If page pointer is zero then we have no block
    cbz(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, &NoBlock);

    // Steal the page offset
    and_(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, ARMEmitter::Reg::r3, 0x0FFF);

    // Shift the offset by the size of the block cache entry
    add(ARMEmitter::XReg::x0, ARMEmitter::XReg::x0, ARMEmitter::XReg::x1, ARMEmitter::ShiftType::LSL, (int)log2(sizeof(FEXCore::LookupCache::LookupCacheEntry)));

    // The the full LookupCacheEntry with a single LDP.
    // Check the guest address first to ensure it maps to the address we are currently at.
    // This fixes aliasing problems
    ldp<ARMEmitter::IndexType::OFFSET>(ARMEmitter::XReg::x3, ARMEmitter::XReg::x1, ARMEmitter::Reg::r0, 0);

    // If the guest address doesn't match, Compile the block.
    cmp(ARMEmitter::XReg::x1, RipReg);
    b(ARMEmitter::Condition::CC_NE, &NoBlock);

    // Check the host address to see if it matches, else compile the block.
    cbz(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r3, &NoBlock);

    // If we've made it here then we have a real compiled block
    {
      // update L1 cache
      ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, Pointers.Common.L1Pointer));

      and_(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, RipReg.R(), LookupCache::L1_ENTRIES_MASK);
      add(ARMEmitter::XReg::x0, ARMEmitter::XReg::x0, ARMEmitter::XReg::x1, ARMEmitter::ShiftType::LSL, 4);
      stp<ARMEmitter::IndexType::OFFSET>(ARMEmitter::XReg::x3, ARMEmitter::XReg::x2, ARMEmitter::Reg::r0);

      // Jump to the block
      br(ARMEmitter::Reg::r3);
    }
  }

  {
    ThreadStopHandlerAddressSpillSRA = GetCursorAddress<uint64_t>();
    if (config.StaticRegisterAllocation)
      SpillStaticRegs(TMP1);

    ThreadStopHandlerAddress = GetCursorAddress<uint64_t>();

    PopCalleeSavedRegisters();

    // Return from the function
    // LR is set to the correct return location now
    ret();
  }

  {
    ExitFunctionLinkerAddress = GetCursorAddress<uint64_t>();
    if (config.StaticRegisterAllocation)
      SpillStaticRegs(TMP1);

    ldr(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));
    add(ARMEmitter::Size::i64Bit, ARMEmitter::XReg::x0, ARMEmitter::XReg::x0, 1);
    str(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));

    mov(ARMEmitter::XReg::x0, STATE);
    mov(ARMEmitter::XReg::x1, ARMEmitter::XReg::lr);

    ldr(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, Pointers.Common.ExitFunctionLink));
#ifdef VIXL_SIMULATOR
    GenerateIndirectRuntimeCall<uintptr_t, void *, void *>(ARMEmitter::Reg::r2);
#else
    blr(ARMEmitter::Reg::r2);
#endif

    if (config.StaticRegisterAllocation)
      FillStaticRegs();

    ldr(ARMEmitter::XReg::x1, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));
    subs(ARMEmitter::Size::i64Bit, ARMEmitter::XReg::x1, ARMEmitter::XReg::x1, 1);
    str(ARMEmitter::XReg::x1, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));

    // Trigger segfault if any deferred signals are pending
    ldr(ARMEmitter::XReg::x1, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalFaultAddress));
    str(ARMEmitter::XReg::zr, ARMEmitter::XReg::x1, 0);

    br(ARMEmitter::Reg::r0);
  }

  // Need to create the block
  {
    Bind(&NoBlock);

    if (config.StaticRegisterAllocation)
      SpillStaticRegs(TMP1);

    ldr(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));
    add(ARMEmitter::Size::i64Bit, ARMEmitter::XReg::x0, ARMEmitter::XReg::x0, 1);
    str(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));

    ldr(ARMEmitter::XReg::x0, &l_CTX);
    mov(ARMEmitter::XReg::x1, STATE);
    ldr(ARMEmitter::XReg::x3, &l_CompileBlock);

    // X2 contains our guest RIP
#ifdef VIXL_SIMULATOR
    GenerateIndirectRuntimeCall<void, void *, uint64_t, void *>(ARMEmitter::Reg::r3);
#else
    blr(ARMEmitter::Reg::r3); // { CTX, Frame, RIP}
#endif

    if (config.StaticRegisterAllocation)
      FillStaticRegs();

    ldr(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));
    subs(ARMEmitter::Size::i64Bit, ARMEmitter::XReg::x0, ARMEmitter::XReg::x0, 1);
    str(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));

    // Trigger segfault if any deferred signals are pending
    ldr(TMP1, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalFaultAddress));
    str(ARMEmitter::XReg::zr, TMP1, 0);

    b(&LoopTop);
  }

  {
    SignalHandlerReturnAddress = GetCursorAddress<uint64_t>();

    // Now to get back to our old location we need to do a fault dance
    // We can't use SIGTRAP here since gdb catches it and never gives it to the application!
    hlt(0);
  }

  {
    SignalHandlerReturnAddressRT = GetCursorAddress<uint64_t>();

    // Now to get back to our old location we need to do a fault dance
    // We can't use SIGTRAP here since gdb catches it and never gives it to the application!
    hlt(0);
  }

  {
    // Guest SIGILL handler
    // Needs to be distinct from the SignalHandlerReturnAddress
    GuestSignal_SIGILL = GetCursorAddress<uint64_t>();

    if (config.StaticRegisterAllocation)
      SpillStaticRegs(TMP1);

    hlt(0);
  }

  {
    // Guest SIGTRAP handler
    // Needs to be distinct from the SignalHandlerReturnAddress
    GuestSignal_SIGTRAP  = GetCursorAddress<uint64_t>();

    if (config.StaticRegisterAllocation)
      SpillStaticRegs(TMP1);

    brk(0);
  }

  {
    // Guest Overflow handler
    // Needs to be distinct from the SignalHandlerReturnAddress
    GuestSignal_SIGSEGV = GetCursorAddress<uint64_t>();

    if (config.StaticRegisterAllocation)
      SpillStaticRegs(TMP1);

    // hlt/udf = SIGILL
    // brk = SIGTRAP
    // ??? = SIGSEGV
    // Force a SIGSEGV by loading zero
    if (CTX->ExitOnHLTEnabled()) {
      ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, ReturningStackLocation));
      add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, ARMEmitter::Reg::r0, 0);
      PopCalleeSavedRegisters();
      ret();
    }
    else {
      LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, 0);
      ldr(ARMEmitter::XReg::x1, ARMEmitter::Reg::r1);
    }
  }

  {
    ThreadPauseHandlerAddressSpillSRA = GetCursorAddress<uint64_t>();
    if (config.StaticRegisterAllocation)
      SpillStaticRegs(TMP1);

    ThreadPauseHandlerAddress = GetCursorAddress<uint64_t>();
    // We are pausing, this means the frontend should be waiting for this thread to idle
    // We will have faulted and jumped to this location at this point

    // Call our sleep handler
    ldr(ARMEmitter::XReg::x0, &l_CTX);
    mov(ARMEmitter::XReg::x1, STATE);
    ldr(ARMEmitter::XReg::x2, &l_Sleep);
#ifdef VIXL_SIMULATOR
    GenerateIndirectRuntimeCall<void, void *, void *>(ARMEmitter::Reg::r2);
#else
    blr(ARMEmitter::Reg::r2);
#endif

    PauseReturnInstruction = GetCursorAddress<uint64_t>();
    // Fault to start running again
    hlt(0);
  }

  {
    // The expectation here is that a thunked function needs to call back in to the JIT in a reentrant safe way
    // To do this safely we need to do some state tracking and register saving
    //
    // eg:
    // JIT Call->
    //  Thunk->
    //    Thunk callback->
    //
    // The thunk callback needs to execute JIT code and when it returns, it needs to safely return to the thunk rather than JIT space
    // This is handled by pushing a return address trampoline to the stack so when the guest address returns it hits our custom thunk return
    //  - This will safely return us to the thunk
    //
    // On return to the thunk, the thunk can get whatever its return value is from the thread context depending on ABI handling on its end
    // When the thunk itself returns, it'll do its regular return logic there
    // void ReentrantCallback(FEXCore::Core::InternalThreadState *Thread, uint64_t RIP);
    CallbackPtr = GetCursorAddress<JITCallback>();

    // We expect the thunk to have previously pushed the registers it was using
    PushCalleeSavedRegisters();

    // First thing we need to move the thread state pointer back in to our register
    mov(STATE, ARMEmitter::XReg::x0);

    // Make sure to adjust the refcounter so we don't clear the cache now
    ldr(ARMEmitter::WReg::w2, STATE_PTR(CpuStateFrame, SignalHandlerRefCounter));
    add(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r2, ARMEmitter::Reg::r2, 1);
    str(ARMEmitter::WReg::w2, STATE_PTR(CpuStateFrame, SignalHandlerRefCounter));

    // Now push the callback return trampoline to the guest stack
    // Guest will be misaligned because calling a thunk won't correct the guest's stack once we call the callback from the host
    LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, CTX->X86CodeGen.CallbackReturn);

    ldr(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, State.gregs[X86State::REG_RSP]));
    sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r2, ARMEmitter::Reg::r2, 16);
    str(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, State.gregs[X86State::REG_RSP]));

    // Store the trampoline to the guest stack
    // Guest stack is now correctly misaligned after a regular call instruction
    str(ARMEmitter::XReg::x0, ARMEmitter::Reg::r2, 0);

    // Store RIP to the context state
    str(ARMEmitter::XReg::x1, STATE_PTR(CpuStateFrame, State.rip));

    // load static regs
    if (config.StaticRegisterAllocation)
      FillStaticRegs();

    // Now go back to the regular dispatcher loop
    b(&LoopTop);
  }

  {
    LUDIVHandlerAddress = GetCursorAddress<uint64_t>();

    PushDynamicRegsAndLR(ARMEmitter::Reg::r3);
    SpillStaticRegs(ARMEmitter::Reg::r3);

    ldr(ARMEmitter::XReg::x3, STATE_PTR(CpuStateFrame, Pointers.AArch64.LUDIV));
#ifdef VIXL_SIMULATOR
    GenerateIndirectRuntimeCall<uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
#else
    blr(ARMEmitter::Reg::r3);
#endif
    FillStaticRegs();

    // Result is now in x0
    // Fix the stack and any values that were stepped on
    PopDynamicRegsAndLR();

    // Go back to our code block
    ret();
  }

  {
    LDIVHandlerAddress = GetCursorAddress<uint64_t>();

    PushDynamicRegsAndLR(ARMEmitter::Reg::r3);
    SpillStaticRegs(ARMEmitter::Reg::r3);

    ldr(ARMEmitter::XReg::x3, STATE_PTR(CpuStateFrame, Pointers.AArch64.LDIV));
#ifdef VIXL_SIMULATOR
    GenerateIndirectRuntimeCall<uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
#else
    blr(ARMEmitter::Reg::r3);
#endif
    FillStaticRegs();

    // Result is now in x0
    // Fix the stack and any values that were stepped on
    PopDynamicRegsAndLR();

    // Go back to our code block
    ret();
  }

  {
    LUREMHandlerAddress = GetCursorAddress<uint64_t>();

    PushDynamicRegsAndLR(ARMEmitter::Reg::r3);
    SpillStaticRegs(ARMEmitter::Reg::r3);

    ldr(ARMEmitter::XReg::x3, STATE_PTR(CpuStateFrame, Pointers.AArch64.LUREM));
#ifdef VIXL_SIMULATOR
    GenerateIndirectRuntimeCall<uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
#else
    blr(ARMEmitter::Reg::r3);
#endif
    FillStaticRegs();

    // Result is now in x0
    // Fix the stack and any values that were stepped on
    PopDynamicRegsAndLR();

    // Go back to our code block
    ret();
  }

  {
    LREMHandlerAddress = GetCursorAddress<uint64_t>();

    PushDynamicRegsAndLR(ARMEmitter::Reg::r3);
    SpillStaticRegs(ARMEmitter::Reg::r3);

    ldr(ARMEmitter::XReg::x3, STATE_PTR(CpuStateFrame, Pointers.AArch64.LREM));

#ifdef VIXL_SIMULATOR
    GenerateIndirectRuntimeCall<uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
#else
    blr(ARMEmitter::Reg::r3);
#endif
    FillStaticRegs();

    // Result is now in x0
    // Fix the stack and any values that were stepped on
    PopDynamicRegsAndLR();

    // Go back to our code block
    ret();
  }

  Bind(&l_CTX);
  dc64(reinterpret_cast<uintptr_t>(CTX));
  Bind(&l_Sleep);
  dc64(reinterpret_cast<uint64_t>(SleepThread));
  Bind(&l_CompileBlock);
  dc64(GetCompileBlockPtr());

  Start = reinterpret_cast<uint64_t>(DispatchPtr);
  End = GetCursorAddress<uint64_t>();
  ClearICache(reinterpret_cast<void*>(DispatchPtr), End - reinterpret_cast<uint64_t>(DispatchPtr));

  if (CTX->Config.BlockJITNaming()) {
    fextl::string Name = fextl::fmt::format("Dispatch_{}", FHU::Syscalls::gettid());
    CTX->Symbols.Register(reinterpret_cast<void*>(DispatchPtr), End - reinterpret_cast<uint64_t>(DispatchPtr), Name);
  }
  if (CTX->Config.GlobalJITNaming()) {
    CTX->Symbols.RegisterJITSpace(reinterpret_cast<void*>(DispatchPtr), End - reinterpret_cast<uint64_t>(DispatchPtr));
  }

#ifdef VIXL_DISASSEMBLER
  if (Disassemble() & FEXCore::Config::Disassemble::DISPATCHER) {
    const auto DisasmEnd = GetCursorAddress<const vixl::aarch64::Instruction*>();
    Disasm.DisassembleBuffer(DisasmBegin, DisasmEnd);
  }
#endif
}

#ifdef VIXL_SIMULATOR
void Arm64Dispatcher::ExecuteDispatch(FEXCore::Core::CpuStateFrame *Frame) {
  Simulator.WriteXRegister(0, reinterpret_cast<int64_t>(Frame));
  Simulator.RunFrom(reinterpret_cast<vixl::aarch64::Instruction const*>(DispatchPtr));
}

void Arm64Dispatcher::ExecuteJITCallback(FEXCore::Core::CpuStateFrame *Frame, uint64_t RIP) {
  Simulator.WriteXRegister(0, reinterpret_cast<int64_t>(Frame));
  Simulator.WriteXRegister(1, RIP);
  Simulator.RunFrom(reinterpret_cast<vixl::aarch64::Instruction const*>(CallbackPtr));
}

#endif

size_t Arm64Dispatcher::GenerateGDBPauseCheck(uint8_t *CodeBuffer, uint64_t GuestRIP) {
  FEXCore::ARMEmitter::Emitter emit{CodeBuffer, MaxGDBPauseCheckSize};

  ARMEmitter::ForwardLabel RunBlock;

  // If we have a gdb server running then run in a less efficient mode that checks if we need to exit
  // This happens when single stepping

  static_assert(sizeof(FEXCore::Context::ContextImpl::Config.RunningMode) == 4, "This is expected to be size of 4");
  emit.ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, Thread));
  emit.ldr(ARMEmitter::XReg::x0, ARMEmitter::Reg::r0, offsetof(FEXCore::Core::InternalThreadState, CTX)); // Get Context
  emit.ldr(ARMEmitter::WReg::w0, ARMEmitter::Reg::r0, offsetof(FEXCore::Context::ContextImpl, Config.RunningMode));

  // If the value == 0 then we don't need to stop
  emit.cbz(ARMEmitter::Size::i32Bit, ARMEmitter::Reg::r0, &RunBlock);
  {
    ARMEmitter::ForwardLabel l_GuestRIP;
    // Make sure RIP is syncronized to the context
    emit.ldr(ARMEmitter::XReg::x0, &l_GuestRIP);
    emit.str(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, State.rip));

    // Stop the thread
    emit.ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, Pointers.Common.ThreadPauseHandlerSpillSRA));
    emit.br(ARMEmitter::Reg::r0);
    emit.Bind(&l_GuestRIP);
    emit.dc64(GuestRIP);
  }
  emit.Bind(&RunBlock);

  auto UsedBytes = emit.GetCursorOffset();
  emit.ClearICache(CodeBuffer, UsedBytes);
  return UsedBytes;
}

size_t Arm64Dispatcher::GenerateInterpreterTrampoline(uint8_t *CodeBuffer) {
  LOGMAN_THROW_AA_FMT(!config.StaticRegisterAllocation, "GenerateInterpreterTrampoline dispatcher does not support SRA");

  FEXCore::ARMEmitter::Emitter emit{CodeBuffer, MaxInterpreterTrampolineSize};
  ARMEmitter::ForwardLabel InlineIRData;

  emit.mov(ARMEmitter::XReg::x0, STATE);
  emit.adr(ARMEmitter::Reg::r1, &InlineIRData);

  emit.ldr(ARMEmitter::XReg::x3, STATE_PTR(CpuStateFrame, Pointers.Interpreter.FragmentExecuter));
  emit.blr(ARMEmitter::Reg::r3);

  emit.ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, Pointers.Common.DispatcherLoopTop));
  emit.br(ARMEmitter::Reg::r0);

  emit.Bind(&InlineIRData);

  auto UsedBytes = emit.GetCursorOffset();
  emit.ClearICache(CodeBuffer, UsedBytes);
  return UsedBytes;
}

void Arm64Dispatcher::InitThreadPointers(FEXCore::Core::InternalThreadState *Thread) {
  // Setup dispatcher specific pointers that need to be accessed from JIT code
  {
    auto &Common = Thread->CurrentFrame->Pointers.Common;

    Common.DispatcherLoopTop = AbsoluteLoopTopAddress;
    Common.DispatcherLoopTopFillSRA = AbsoluteLoopTopAddressFillSRA;
    Common.ExitFunctionLinker = ExitFunctionLinkerAddress;
    Common.ThreadStopHandlerSpillSRA = ThreadStopHandlerAddressSpillSRA;
    Common.ThreadPauseHandlerSpillSRA = ThreadPauseHandlerAddressSpillSRA;
    Common.GuestSignal_SIGILL = GuestSignal_SIGILL;
    Common.GuestSignal_SIGTRAP = GuestSignal_SIGTRAP;
    Common.GuestSignal_SIGSEGV = GuestSignal_SIGSEGV;
    Common.SignalReturnHandler = SignalHandlerReturnAddress;
    Common.SignalReturnHandlerRT = SignalHandlerReturnAddressRT;

    auto &AArch64 = Thread->CurrentFrame->Pointers.AArch64;
    AArch64.LUDIVHandler = LUDIVHandlerAddress;
    AArch64.LDIVHandler = LDIVHandlerAddress;
    AArch64.LUREMHandler = LUREMHandlerAddress;
    AArch64.LREMHandler = LREMHandlerAddress;
  }
}

fextl::unique_ptr<Dispatcher> Dispatcher::CreateArm64(FEXCore::Context::ContextImpl *CTX, const DispatcherConfig &Config) {
  return fextl::make_unique<Arm64Dispatcher>(CTX, Config);
}

}
