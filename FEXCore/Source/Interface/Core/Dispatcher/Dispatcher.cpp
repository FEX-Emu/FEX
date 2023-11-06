// SPDX-License-Identifier: MIT

#include "Interface/Context/Context.h"
#include "Interface/Core/ArchHelpers/CodeEmitter/Emitter.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/LookupCache.h"
#include "Interface/Core/X86HelperGen.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <cstring>
#include <signal.h>

namespace FEXCore::CPU {

void Dispatcher::SleepThread(FEXCore::Context::ContextImpl *ctx, FEXCore::Core::CpuStateFrame *Frame) {
  auto Thread = Frame->Thread;

  --ctx->IdleWaitRefCount;
  ctx->IdleWaitCV.notify_all();

  Thread->RunningEvents.ThreadSleeping = true;

  // Go to sleep
  Thread->StartRunning.Wait();

  Thread->RunningEvents.Running = true;
  ++ctx->IdleWaitRefCount;
  Thread->RunningEvents.ThreadSleeping = false;

  ctx->IdleWaitCV.notify_all();
}

uint64_t Dispatcher::GetCompileBlockPtr() {
  using ClassPtrType = void (FEXCore::Context::ContextImpl::*)(FEXCore::Core::CpuStateFrame *, uint64_t);
  union PtrCast {
    ClassPtrType ClassPtr;
    uintptr_t Data;
  };

  PtrCast CompileBlockPtr;
  CompileBlockPtr.ClassPtr = &FEXCore::Context::ContextImpl::CompileBlockJit;
  return CompileBlockPtr.Data;
}

constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096 * 2;

Dispatcher::Dispatcher(FEXCore::Context::ContextImpl *ctx, const DispatcherConfig &config)
  : Arm64Emitter(ctx, FEXCore::Allocator::VirtualAlloc(MAX_DISPATCHER_CODE_SIZE, true), MAX_DISPATCHER_CODE_SIZE)
  , CTX {ctx}
  , config {config} {
  EmitDispatcher();
}

Dispatcher::~Dispatcher() {
  auto BufferSize = GetBufferSize();
  if (BufferSize) {
    FEXCore::Allocator::VirtualFree(GetBufferBase(), BufferSize);
  }
}

void Dispatcher::EmitDispatcher() {
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
  sub(ARMEmitter::XReg::x0, ARMEmitter::XReg::x0, RipReg);
  cbnz(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r0, &FullLookup);

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
    sub(ARMEmitter::XReg::x1, ARMEmitter::XReg::x1, RipReg);
    cbnz(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, &NoBlock);

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
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<uintptr_t, void *, void *>(ARMEmitter::Reg::r2);
    }
    else {
      blr(ARMEmitter::Reg::r2);
    }

    if (config.StaticRegisterAllocation)
      FillStaticRegs();

    ldr(ARMEmitter::XReg::x1, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));
    sub(ARMEmitter::Size::i64Bit, ARMEmitter::XReg::x1, ARMEmitter::XReg::x1, 1);
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
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<void, void *, uint64_t, void *>(ARMEmitter::Reg::r3);
    }
    else {
      blr(ARMEmitter::Reg::r3); // { CTX, Frame, RIP}
    }

    if (config.StaticRegisterAllocation)
      FillStaticRegs();

    ldr(ARMEmitter::XReg::x0, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));
    sub(ARMEmitter::Size::i64Bit, ARMEmitter::XReg::x0, ARMEmitter::XReg::x0, 1);
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
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<void, void *, void *>(ARMEmitter::Reg::r2);
    }
    else {
      blr(ARMEmitter::Reg::r2);
    }

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
    sub(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r2, ARMEmitter::Reg::r2, CTX->Config.Is64BitMode ? 16 : 12);
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
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
    }
    else {
      blr(ARMEmitter::Reg::r3);
    }
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
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
    }
    else {
      blr(ARMEmitter::Reg::r3);
    }
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
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
    }
    else {
      blr(ARMEmitter::Reg::r3);
    }
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

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<uint64_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
    }
    else {
      blr(ARMEmitter::Reg::r3);
    }
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
    CTX->Symbols.RegisterNamedRegion(reinterpret_cast<void*>(DispatchPtr), End - reinterpret_cast<uint64_t>(DispatchPtr), Name);
  }
  if (CTX->Config.GlobalJITNaming()) {
    CTX->Symbols.RegisterJITSpace(reinterpret_cast<void*>(DispatchPtr), End - reinterpret_cast<uint64_t>(DispatchPtr));
  }

#ifdef VIXL_DISASSEMBLER
  if (Disassemble() & FEXCore::Config::Disassemble::DISPATCHER) {
    const auto DisasmEnd = GetCursorAddress<const vixl::aarch64::Instruction*>();
    for (auto PCToDecode = DisasmBegin; PCToDecode < DisasmEnd; PCToDecode += 4) {
      DisasmDecoder->Decode(PCToDecode);
      auto Output = Disasm.GetOutput();
      LogMan::Msg::IFmt("{}", Output);
    }
  }
#endif
}

#ifdef VIXL_SIMULATOR
void Dispatcher::ExecuteDispatch(FEXCore::Core::CpuStateFrame *Frame) {
  Simulator.WriteXRegister(0, reinterpret_cast<int64_t>(Frame));
  Simulator.RunFrom(reinterpret_cast<vixl::aarch64::Instruction const*>(DispatchPtr));
}

void Dispatcher::ExecuteJITCallback(FEXCore::Core::CpuStateFrame *Frame, uint64_t RIP) {
  Simulator.WriteXRegister(0, reinterpret_cast<int64_t>(Frame));
  Simulator.WriteXRegister(1, RIP);
  Simulator.RunFrom(reinterpret_cast<vixl::aarch64::Instruction const*>(CallbackPtr));
}

#endif

void Dispatcher::InitThreadPointers(FEXCore::Core::InternalThreadState *Thread) {
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

fextl::unique_ptr<Dispatcher> Dispatcher::Create(FEXCore::Context::ContextImpl *CTX, const DispatcherConfig &Config) {
  return fextl::make_unique<Dispatcher>(CTX, Config);
}

}
