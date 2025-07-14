// SPDX-License-Identifier: MIT

#include "Common/SoftFloat.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/LookupCache.h"
#include "Interface/Core/X86HelperGen.h"
#include "Utils/MemberFunctionToPointer.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/HLE/SyscallHandler.h>
#include <FEXCore/Utils/Event.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>

#include <CodeEmitter/Emitter.h>

#include <atomic>
#include <condition_variable>
#include <csignal>
#include <cstring>
#include <signal.h>

namespace FEXCore::CPU {

static void SleepThread(FEXCore::Context::ContextImpl* CTX, FEXCore::Core::CpuStateFrame* Frame) {
  CTX->SyscallHandler->SleepThread(CTX, Frame);
}

constexpr size_t MAX_DISPATCHER_CODE_SIZE = 4096 * 4;

Dispatcher::Dispatcher(FEXCore::Context::ContextImpl* ctx)
  : Arm64Emitter(ctx, FEXCore::Allocator::VirtualAlloc(MAX_DISPATCHER_CODE_SIZE, true), MAX_DISPATCHER_CODE_SIZE)
  , CTX {ctx} {
  EmitDispatcher();
}

Dispatcher::~Dispatcher() {
  auto BufferSize = GetBufferSize();
  if (BufferSize) {
    FEXCore::Allocator::VirtualFree(GetBufferBase(), BufferSize);
  }
}

void Dispatcher::EmitDispatcher() {
  // Don't modify TMP3 since it contains our RIP once the block doesn't exist
  auto RipReg = TMP3;
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
  ARMEmitter::ForwardLabel l_CompileSingleStep;

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

  ARMEmitter::ForwardLabel CompileSingleStep;
  AbsoluteLoopTopAddressFillSRA = GetCursorAddress<uint64_t>();

  FillStaticRegs();
  ldr(RipReg, STATE_PTR(CpuStateFrame, State.rip));
  cbnz(ARMEmitter::Size::i32Bit, ENTRY_FILL_SRA_SINGLE_INST_REG, &CompileSingleStep);

  ARMEmitter::BiDirectionalLabel LoopTop {};

#ifdef _M_ARM_64EC
  b(&LoopTop);

  AbsoluteLoopTopAddressEnterECFillSRA = GetCursorAddress<uint64_t>();
  ldr(STATE, EC_ENTRY_CPUAREA_REG, CPU_AREA_EMULATOR_DATA_OFFSET);
  FillStaticRegs();

  ldr(RipReg, STATE_PTR(CpuStateFrame, State.rip));
  // Force a single instruction block if ENTRY_FILL_SRA_SINGLE_INST_REG is nonzero entering the JIT, used for inline SMC handling.
  cbnz(ARMEmitter::Size::i32Bit, ENTRY_FILL_SRA_SINGLE_INST_REG, &CompileSingleStep);

  // Enter JIT
  b(&LoopTop);

  AbsoluteLoopTopAddressEnterEC = GetCursorAddress<uint64_t>();
  // Load ThreadState and write the target PC there
  ldr(STATE, EC_ENTRY_CPUAREA_REG, CPU_AREA_EMULATOR_DATA_OFFSET);
  str(EC_CALL_CHECKER_PC_REG, STATE_PTR(CpuStateFrame, State.rip));

  // Swap stacks to the emulator stack
  ldr(TMP1, EC_ENTRY_CPUAREA_REG, CPU_AREA_EMULATOR_STACK_BASE_OFFSET);
  add(ARMEmitter::Size::i64Bit, StaticRegisters[X86State::REG_RSP], ARMEmitter::Reg::rsp, 0);
  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, TMP1, 0);

  ldr(REG_CALLRET_SP, STATE_PTR(CpuStateFrame, State.callret_sp));

  FillSpecialRegs(TMP1, TMP2, false, true);

  // As ARM64EC uses this as an entrypoint for both guest calls and host returns, opportunistically try to return
  // using the call-ret stack to avoid unbalancing it.
  ldp<ARMEmitter::IndexType::OFFSET>(TMP1, TMP2, REG_CALLRET_SP);
  // EC_CALL_CHECKER_PC_REG is REG_PF which isn't touched by any of the above
  sub(ARMEmitter::Size::i64Bit, TMP1, EC_CALL_CHECKER_PC_REG, TMP1);
  cbnz(ARMEmitter::Size::i64Bit, TMP1, &LoopTop);

  // If the entry at the TOS is for the target address, pop it and return to the JIT code
  add(ARMEmitter::Size::i64Bit, REG_CALLRET_SP, REG_CALLRET_SP, 0x10);
  ret(TMP2);

  // Enter JIT
#endif

  // We want to ensure that we are 16 byte aligned at the top of this loop
  Align16B();
  ARMEmitter::BiDirectionalLabel FullLookup {};
  ARMEmitter::BiDirectionalLabel CallBlock {};

  Bind(&LoopTop);
  AbsoluteLoopTopAddress = GetCursorAddress<uint64_t>();

  // Load in our RIP
  ldr(RipReg, STATE_PTR(CpuStateFrame, State.rip));

  ldrb(TMP1, STATE_PTR(CpuStateFrame, State.flags[X86State::RFLAG_TF_RAW_LOC]));
  cbnz(ARMEmitter::Size::i32Bit, TMP1, &CompileSingleStep);

  // L1 Cache
  ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.L1Pointer));

  and_(ARMEmitter::Size::i64Bit, TMP4, RipReg.R(), LookupCache::L1_ENTRIES_MASK);
  add(ARMEmitter::Size::i64Bit, TMP1, TMP1, TMP4, ARMEmitter::ShiftType::LSL, 4);
  ldp<ARMEmitter::IndexType::OFFSET>(TMP4, TMP1, TMP1, 0);
  sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, RipReg);
  cbnz(ARMEmitter::Size::i64Bit, TMP1, &FullLookup);

  br(TMP4);

  // L1C check failed, do a full lookup
  Bind(&FullLookup);

  // This is the block cache lookup routine
  // It matches what is going on it LookupCache.h::FindBlock
  ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.L2Pointer));

  // Mask the address by the virtual address size so we can check for aliases
  uint64_t VirtualMemorySize = CTX->Config.VirtualMemSize;
  if (std::popcount(VirtualMemorySize) == 1) {
    and_(ARMEmitter::Size::i64Bit, TMP4, RipReg.R(), VirtualMemorySize - 1);
  } else {
    LoadConstant(ARMEmitter::Size::i64Bit, TMP4, VirtualMemorySize);
    and_(ARMEmitter::Size::i64Bit, TMP4, RipReg.R(), TMP4);
  }

  ARMEmitter::ForwardLabel NoBlock;

  {
    // Offset the address and add to our page pointer
    lsr(ARMEmitter::Size::i64Bit, TMP2, TMP4, 12);

    // Load the pointer from the offset
    ldr(TMP1, TMP1, TMP2, ARMEmitter::ExtendedType::LSL_64, 3);

    // If page pointer is zero then we have no block
    cbz(ARMEmitter::Size::i64Bit, TMP1, &NoBlock);

    // Steal the page offset
    and_(ARMEmitter::Size::i64Bit, TMP2, TMP4, 0x0FFF);

    // Shift the offset by the size of the block cache entry
    add(TMP1, TMP1, TMP2, ARMEmitter::ShiftType::LSL, (int)log2(sizeof(FEXCore::LookupCache::LookupCacheEntry)));

    // The the full LookupCacheEntry with a single LDP.
    // Check the guest address first to ensure it maps to the address we are currently at.
    // This fixes aliasing problems
    ldp<ARMEmitter::IndexType::OFFSET>(TMP4, TMP2, TMP1, 0);

    // If the guest address doesn't match, Compile the block.
    sub(TMP2, TMP2, RipReg);
    cbnz(ARMEmitter::Size::i64Bit, TMP2, &NoBlock);

    // Check the host address to see if it matches, else compile the block.
    cbz(ARMEmitter::Size::i64Bit, TMP4, &NoBlock);

    // If we've made it here then we have a real compiled block
    {
      // update L1 cache
      ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.Common.L1Pointer));

      and_(ARMEmitter::Size::i64Bit, TMP2, RipReg.R(), LookupCache::L1_ENTRIES_MASK);
      add(TMP1, TMP1, TMP2, ARMEmitter::ShiftType::LSL, 4);
      stp<ARMEmitter::IndexType::OFFSET>(TMP4, RipReg, TMP1);

      // Jump to the block
      br(TMP4);
    }
  }

  {
    ThreadStopHandlerAddressSpillSRA = GetCursorAddress<uint64_t>();
    SpillStaticRegs(TMP1);

    ThreadStopHandlerAddress = GetCursorAddress<uint64_t>();

    PopCalleeSavedRegisters();

    // Return from the function
    // LR is set to the correct return location now
    ret();
  }

  // Clobbers TMP1/2
  auto EmitSignalGuardedRegion = [&](auto Body) {
#ifndef _WIN32
    ldr(TMP2, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));
    add(ARMEmitter::Size::i64Bit, TMP2, TMP2, 1);
    str(TMP2, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));
#endif

#ifdef _M_ARM_64EC
    ldr(TMP2, ARMEmitter::XReg::x18, TEB_CPU_AREA_OFFSET);
    LoadConstant(ARMEmitter::Size::i32Bit, TMP1, 1);
    strb(TMP1.W(), TMP2, CPU_AREA_IN_SYSCALL_CALLBACK_OFFSET);
#endif

    Body();

#ifdef _M_ARM_64EC
    ldr(TMP2, ARMEmitter::XReg::x18, TEB_CPU_AREA_OFFSET);
    strb(ARMEmitter::WReg::zr, TMP2, CPU_AREA_IN_SYSCALL_CALLBACK_OFFSET);
#endif

#ifndef _WIN32
    ldr(TMP2, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));
    sub(ARMEmitter::Size::i64Bit, TMP2, TMP2, 1);
    str(TMP2, STATE, offsetof(FEXCore::Core::CPUState, DeferredSignalRefCount));

    // Trigger segfault if any deferred signals are pending
    strb(ARMEmitter::XReg::zr, STATE,
         offsetof(FEXCore::Core::InternalThreadState, InterruptFaultPage) - offsetof(FEXCore::Core::InternalThreadState, BaseFrameState));
#endif
  };

  {
    ExitFunctionLinkerAddress = GetCursorAddress<uint64_t>();
    EmitSignalGuardedRegion([&]() {
      SpillStaticRegs(TMP1);

      mov(ARMEmitter::XReg::x0, STATE);
      mov(ARMEmitter::XReg::x1, ARMEmitter::XReg::lr);

      ldr(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, Pointers.Common.ExitFunctionLink));
      if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
        GenerateIndirectRuntimeCall<uintptr_t, void*, void*>(ARMEmitter::Reg::r2);
      } else {
        blr(ARMEmitter::Reg::r2);
      }

      if (!TMP_ABIARGS) {
        mov(TMP1, ARMEmitter::XReg::x0);
      }

      FillStaticRegs();
    });

    br(TMP1);
  }

#ifdef _M_ARM_64EC
  // Clobbers TMP1/2
  auto EmitECExitCheck = [&]() {
    // Check the EC code bitmap incase we need to exit the JIT to call into native code.
    ARMEmitter::ForwardLabel l_NotECCode;
    ldr(TMP1, ARMEmitter::XReg::x18, TEB_PEB_OFFSET);
    ldr(TMP1, TMP1, PEB_EC_CODE_BITMAP_OFFSET);

    lsr(ARMEmitter::Size::i64Bit, TMP2, RipReg, 15);
    and_(ARMEmitter::Size::i64Bit, TMP2, TMP2, 0x1fffffffffff8);
    ldr(TMP1, TMP1, TMP2, ARMEmitter::ExtendedType::LSL_64, 0);
    lsr(ARMEmitter::Size::i64Bit, TMP2, RipReg, 12);
    lsrv(ARMEmitter::Size::i64Bit, TMP1, TMP1, TMP2);
    tbz(TMP1, 0, &l_NotECCode);

    str(REG_CALLRET_SP, STATE_PTR(CpuStateFrame, State.callret_sp));

    add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, StaticRegisters[X86State::REG_RSP], 0);
    mov(EC_CALL_CHECKER_PC_REG, RipReg);
    ldr(TMP2, STATE_PTR(CpuStateFrame, Pointers.Common.ExitFunctionEC));
    br(TMP2);

    Bind(&l_NotECCode);
  };
#endif

  // Need to create the block
  {
    Bind(&NoBlock);

#ifdef _M_ARM_64EC
    EmitECExitCheck();
#endif

    EmitSignalGuardedRegion([&]() {
      SpillStaticRegs(TMP1);

      if (!TMP_ABIARGS) {
        mov(ARMEmitter::XReg::x2, RipReg);
      }

      ldr(ARMEmitter::XReg::x0, &l_CTX);
      mov(ARMEmitter::XReg::x1, STATE);
      // x2 contains guest RIP
      mov(ARMEmitter::XReg::x3, 0);
      ldr(ARMEmitter::XReg::x4, &l_CompileBlock);

      if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
        GenerateIndirectRuntimeCall<uintptr_t, void*, void*, uint64_t, uint64_t>(ARMEmitter::Reg::r4);
      } else {
        blr(ARMEmitter::Reg::r4); // { CTX, Frame, RIP, MaxInst }
      }

      // Result is now in x0
      if (!TMP_ABIARGS) {
        mov(TMP1, ARMEmitter::XReg::x0);
      }

      FillStaticRegs();
    });

    // Jump to the compiled block
    br(TMP1);
  }

  {
    Bind(&CompileSingleStep);

#ifdef _M_ARM_64EC
    EmitECExitCheck();
#endif

    EmitSignalGuardedRegion([&]() {
      SpillStaticRegs(TMP1);

      if (!TMP_ABIARGS) {
        mov(ARMEmitter::XReg::x2, RipReg);
      }

      ldr(ARMEmitter::XReg::x0, &l_CTX);
      mov(ARMEmitter::XReg::x1, STATE);
      // x2 contains guest RIP
      ldr(ARMEmitter::XReg::x4, &l_CompileSingleStep);

      if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
        GenerateIndirectRuntimeCall<uintptr_t, void*, void*, uint64_t, uint64_t>(ARMEmitter::Reg::r4);
      } else {
        blr(ARMEmitter::Reg::r4); // { CTX, Frame, RIP }
      }

      // Result is now in x0
      if (!TMP_ABIARGS) {
        mov(TMP1, ARMEmitter::XReg::x0);
      }

      FillStaticRegs();
    });

    // Jump to the compiled block
    br(TMP1);
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

    SpillStaticRegs(TMP1);

    hlt(0);
  }

  {
    // Guest SIGTRAP handler
    // Needs to be distinct from the SignalHandlerReturnAddress
    GuestSignal_SIGTRAP = GetCursorAddress<uint64_t>();

    SpillStaticRegs(TMP1);

    brk(0);
  }

  {
    // Guest Overflow handler
    // Needs to be distinct from the SignalHandlerReturnAddress
    GuestSignal_SIGSEGV = GetCursorAddress<uint64_t>();

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
    } else {
      LoadConstant(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, 0);
      ldr(ARMEmitter::XReg::x1, ARMEmitter::Reg::r1);
    }
  }

  {
    ThreadPauseHandlerAddressSpillSRA = GetCursorAddress<uint64_t>();
    SpillStaticRegs(TMP1);

    ThreadPauseHandlerAddress = GetCursorAddress<uint64_t>();
    // We are pausing, this means the frontend should be waiting for this thread to idle
    // We will have faulted and jumped to this location at this point

    // Call our sleep handler
    ldr(ARMEmitter::XReg::x0, &l_CTX);
    mov(ARMEmitter::XReg::x1, STATE);
    ldr(ARMEmitter::XReg::x2, &l_Sleep);
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<void, void*, void*>(ARMEmitter::Reg::r2);
    } else {
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
    FillStaticRegs();

    // Now go back to the regular dispatcher loop
    b(&LoopTop);
  }

  auto EmitLongALUOpHandler = [&](auto R, auto Offset) {
    auto Address = GetCursorAddress<uint64_t>();

    PushDynamicRegs(TMP4);
    SpillStaticRegs(TMP4);

    if (!TMP_ABIARGS) {
      mov(ARMEmitter::XReg::x0, TMP1);
      mov(ARMEmitter::XReg::x1, TMP2);
      mov(ARMEmitter::XReg::x2, TMP3);
    }

    ldr(ARMEmitter::XReg::x3, R, Offset);
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<__uint128_t, uint64_t, uint64_t, uint64_t>(ARMEmitter::Reg::r3);
    } else {
      blr(ARMEmitter::Reg::r3);
    }

    // Result is now in x0, x1
    if (!TMP_ABIARGS) {
      mov(TMP1, ARMEmitter::XReg::x0);
      mov(TMP2, ARMEmitter::XReg::x1);
    }

    FillStaticRegs();

    // Fix the stack and any values that were stepped on
    PopDynamicRegs();

    // Go back to our code block
    ret();
    return Address;
  };

  LUDIVHandlerAddress = EmitLongALUOpHandler(STATE_PTR(CpuStateFrame, Pointers.AArch64.LUDIV));
  LDIVHandlerAddress = EmitLongALUOpHandler(STATE_PTR(CpuStateFrame, Pointers.AArch64.LDIV));

  // Interpreter fallbacks
  {
    constexpr static std::array<FallbackABI, FABI_UNKNOWN> ABIS {{
      FABI_F80_I16_F32_PTR,
      FABI_F80_I16_F64_PTR,
      FABI_F80_I16_I16_PTR,
      FABI_F80_I16_I32_PTR,
      FABI_F32_I16_F80_PTR,
      FABI_F64_I16_F80_PTR,
      FABI_F64_I16_F64_PTR,
      FABI_F64_I16_F64_F64_PTR,
      FABI_I16_I16_F80_PTR,
      FABI_I32_I16_F80_PTR,
      FABI_I64_I16_F80_PTR,
      FABI_I64_I16_F80_F80_PTR,
      FABI_F80_I16_F80_PTR,
      FABI_F80_I16_F80_F80_PTR,
      FABI_F80x2_I16_F80_PTR,
      FABI_F64x2_I16_F64_PTR,
      FABI_I32_I64_I64_V128_V128_I16,
      FABI_I32_V128_V128_I16,
    }};

    for (auto ABI : ABIS) {
      ABIPointers[ABI] = GenerateABICall(ABI);
    }
  }

  Bind(&l_CTX);
  dc64(reinterpret_cast<uintptr_t>(CTX));
  Bind(&l_Sleep);
  dc64(reinterpret_cast<uint64_t>(SleepThread));
  Bind(&l_CompileBlock);
  FEXCore::Utils::MemberFunctionToPointerCast PMFCompileBlock(&FEXCore::Context::ContextImpl::CompileBlock);
  dc64(PMFCompileBlock.GetConvertedPointer());
  Bind(&l_CompileSingleStep);
  FEXCore::Utils::MemberFunctionToPointerCast PMFCompileSingleStep(&FEXCore::Context::ContextImpl::CompileSingleStep);
  dc64(PMFCompileSingleStep.GetConvertedPointer());

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
      auto Output = Disasm->GetOutput();
      LogMan::Msg::IFmt("{}", Output);
    }
  }
#endif
}

#ifdef VIXL_SIMULATOR
void Dispatcher::ExecuteDispatch(FEXCore::Core::CpuStateFrame* Frame) {
  Simulator.WriteXRegister(0, reinterpret_cast<int64_t>(Frame));
  Simulator.WriteXRegister(1, 0);
  Simulator.RunFrom(reinterpret_cast< const vixl::aarch64::Instruction*>(DispatchPtr));
}

void Dispatcher::ExecuteJITCallback(FEXCore::Core::CpuStateFrame* Frame, uint64_t RIP) {
  Simulator.WriteXRegister(0, reinterpret_cast<int64_t>(Frame));
  Simulator.WriteXRegister(1, RIP);
  Simulator.RunFrom(reinterpret_cast< const vixl::aarch64::Instruction*>(CallbackPtr));
}

#endif

uint64_t Dispatcher::GenerateABICall(FallbackABI ABI) {
  auto Address = GetCursorAddress<uint64_t>();
  constexpr static auto FallbackPointerReg = TMP4;
  constexpr static auto ABI1 = ARMEmitter::XReg::x0;
  constexpr static auto ABI2 = ARMEmitter::XReg::x1;
  constexpr static auto ABI3 = ARMEmitter::XReg::x2;

  constexpr static auto VABI1 = ARMEmitter::VReg::v0;
  constexpr static auto VABI2 = ARMEmitter::VReg::v1;

  auto FillF80x2Result = [&]() {
    if (!TMP_ABIARGS) {
      mov(VTMP1.Q(), VABI1.Q());
      mov(VTMP2.Q(), VABI2.Q());
    }
    FillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, true);
  };

  auto FillF64x2Result = [&]() {
    if (!TMP_ABIARGS) {
      fmov(VTMP1.D(), VABI1.D());
      fmov(VTMP2.D(), VABI2.D());
    }
    FillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, true);
  };

  auto FillF80Result = [&]() {
    if (VTMP1 != VABI1) {
      mov(VTMP1.Q(), VABI1.Q());
    }
    FillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, true);
  };

  auto FillF64Result = [&]() {
    if (!TMP_ABIARGS) {
      fmov(VTMP1.D(), VABI1.D());
    }
    FillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, true);
  };

  auto FillF32Result = [&]() {
    if (!TMP_ABIARGS) {
      fmov(VTMP1.S(), VABI1.S());
    }
    FillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, true);
  };

  auto FillI64Result = [&]() {
    if (!TMP_ABIARGS) {
      mov(TMP1, ARMEmitter::XReg::x0);
    }
    FillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, true);
  };

  auto FillI32Result = [&]() {
    if (!TMP_ABIARGS) {
      mov(TMP1.W(), ARMEmitter::WReg::w0);
    }
    FillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, true);
  };

  auto FillI16Result = [&]() {
    if (!TMP_ABIARGS) {
      mov(TMP1, ARMEmitter::XReg::x0);
    }
    FillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, true);
  };

  switch (ABI) {
  case FABI_F80_I16_F32_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): source
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    if (!TMP_ABIARGS) {
      fmov(VABI1.S(), VTMP1.S());
    }
    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    mov(ARMEmitter::XReg::x1, STATE);
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<FEXCore::VectorRegType, uint16_t, float, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF80Result();
  } break;
  case FABI_F80_I16_F64_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): source
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    if (!TMP_ABIARGS) {
      fmov(VABI1.D(), VTMP1.D());
    }
    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    mov(ARMEmitter::XReg::x1, STATE);
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<FEXCore::VectorRegType, uint16_t, double, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF80Result();
  } break;
  case FABI_F80_I16_I16_PTR:
  case FABI_F80_I16_I32_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // tmp2 (x1/x11): source
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    if (!TMP_ABIARGS) {
      mov(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::r1, TMP2);
    }
    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    mov(ARMEmitter::XReg::x2, STATE);
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<FEXCore::VectorRegType, uint16_t, uint32_t, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF80Result();
  } break;
  case FABI_F32_I16_F80_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): source
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    if (!TMP_ABIARGS) {
      fmov(VABI1.D(), VTMP1.D());
    }

    mov(ARMEmitter::XReg::x1, STATE);
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<float, uint16_t, FEXCore::VectorRegType, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF32Result();
  } break;
  case FABI_F64_I16_F80_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): source
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    if (!TMP_ABIARGS) {
      mov(VABI1.Q(), VTMP1.Q());
    }
    mov(ARMEmitter::XReg::x1, STATE);

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<double, uint16_t, FEXCore::VectorRegType, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF64Result();
  } break;
  case FABI_F64_I16_F64_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): vector source
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    if (!TMP_ABIARGS) {
      fmov(VABI1.D(), VTMP1.D());
    }
    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    mov(ARMEmitter::XReg::x1, STATE);

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<double, uint16_t, double, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF64Result();
  } break;
  case FABI_F64_I16_F64_F64_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): vector source 1
    // vtmp2 (v1/v17): vector source 2
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    if (!TMP_ABIARGS) {
      fmov(VABI1.D(), VTMP1.D());
      fmov(VABI2.D(), VTMP2.D());
    }

    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    mov(ARMEmitter::XReg::x1, STATE);
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<double, uint16_t, double, double, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF64Result();
  } break;
  case FABI_I16_I16_F80_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): source
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    if (!TMP_ABIARGS) {
      mov(VABI1.Q(), VTMP1.Q());
    }
    mov(ARMEmitter::XReg::x1, STATE);

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<uint32_t, uint16_t, FEXCore::VectorRegType, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillI16Result();
  } break;
  case FABI_I32_I16_F80_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): source
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    if (!TMP_ABIARGS) {
      mov(VABI1.Q(), VTMP1.Q());
    }
    mov(ARMEmitter::XReg::x1, STATE);

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<uint32_t, uint16_t, FEXCore::VectorRegType, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillI32Result();
  } break;
  case FABI_I64_I16_F80_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): source
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    if (!TMP_ABIARGS) {
      mov(VABI1.Q(), VTMP1.Q());
    }
    mov(ARMEmitter::XReg::x1, STATE);

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<uint64_t, uint16_t, FEXCore::VectorRegType, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillI64Result();
  } break;
  case FABI_I64_I16_F80_F80_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): vector source 1
    // vtmp2 (v1/v17): vector source 2
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    if (!TMP_ABIARGS) {
      mov(VABI1.Q(), VTMP1.Q());
      mov(VABI2.Q(), VTMP2.Q());
    }
    mov(ARMEmitter::XReg::x1, STATE);

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<uint64_t, uint16_t, FEXCore::VectorRegType, FEXCore::VectorRegType, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillI64Result();
  } break;
  case FABI_F80_I16_F80_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): vector source 1
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    mov(ARMEmitter::XReg::x1, STATE);
    if (!TMP_ABIARGS) {
      mov(VABI1.Q(), VTMP1.Q());
    }

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<FEXCore::VectorRegType, uint16_t, FEXCore::VectorRegType, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF80Result();
  } break;
  case FABI_F80_I16_F80_F80_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): vector source 1
    // vtmp2 (v1/v17): vector source 2
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    if (!TMP_ABIARGS) {
      mov(VABI1.Q(), VTMP1.Q());
      mov(VABI2.Q(), VTMP2.Q());
    }
    mov(ARMEmitter::XReg::x1, STATE);

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<FEXCore::VectorRegType, uint16_t, FEXCore::VectorRegType, FEXCore::VectorRegType, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF80Result();
  } break;
  case FABI_F80x2_I16_F80_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): vector source 1
    // vtmp2 (v1/v16): vector source 2

    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    mov(ARMEmitter::XReg::x1, STATE);
    if (!TMP_ABIARGS) {
      mov(VABI1.Q(), VTMP1.Q());
    }

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      // GenerateIndirectRuntimeCall<FEXCore::VectorRegPairType, uint16_t, FEXCore::VectorRegType, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF80x2Result();
  } break;
  case FABI_F64x2_I16_F64_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): vector source 1
    // vtmp2 (v1/v16): vector source 2

    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    ldrh(ARMEmitter::WReg::w0, STATE, offsetof(FEXCore::Core::CPUState, FCW));
    mov(ARMEmitter::XReg::x1, STATE);
    if (!TMP_ABIARGS) {
      fmov(VABI1.D(), VTMP1.D());
    }

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      // GenerateIndirectRuntimeCall<FEXCore::VectorScalarF64Pair, uint16_t, FEXCore::VectorRegType, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF64x2Result();
  } break;
  case FABI_I32_I64_I64_V128_V128_I16: {
    // Linux Reg/Win32 Reg:
    // stack: FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): vector source 1
    // vtmp2 (v1/v17): vector source 2
    // tmp1 (x0/x10): source 1
    // tmp2 (x1/x11): source 2
    // tmp3 (x2/x12): source 3

    const size_t OriginalSPOffset = SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP4, true);

    // Load the Fallback handler pointer from the stack.
    ldr(FallbackPointerReg, ARMEmitter::XReg::rsp, OriginalSPOffset);

    if (!TMP_ABIARGS) {
      mov(ABI1, TMP1);
      mov(ABI2, TMP2);
      mov(ABI3, TMP3);
      mov(VABI1.Q(), VTMP1.Q());
      mov(VABI2.Q(), VTMP2.Q());
    }

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<uint32_t, uint64_t, uint64_t, FEXCore::VectorRegType, FEXCore::VectorRegType, uint16_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillI32Result();
  } break;
  case FABI_I32_V128_V128_I16: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): vector source 1
    // vtmp2 (v1/v17): vector source 2
    // tmp1 (x0/x10): source 1
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    if (!TMP_ABIARGS) {
      mov(VABI1.Q(), VTMP1.Q());
      mov(VABI2.Q(), VTMP2.Q());
      mov(ABI1, TMP1);
    }

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<uint32_t, FEXCore::VectorRegType, FEXCore::VectorRegType, uint16_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillI32Result();
  } break;
  case FABI_UNKNOWN:
  default:
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED
    LOGMAN_MSG_A_FMT("Unhandled IR Fallback ABI: {}", ToUnderlying(ABI));
#endif
    break;
  }

  // Return to JIT
  ret();

  return Address;
}

void Dispatcher::InitThreadPointers(FEXCore::Core::InternalThreadState* Thread) {
  // Setup dispatcher specific pointers that need to be accessed from JIT code
  {
    auto& Common = Thread->CurrentFrame->Pointers.Common;

    Common.DispatcherLoopTop = AbsoluteLoopTopAddress;
    Common.DispatcherLoopTopFillSRA = AbsoluteLoopTopAddressFillSRA;
    Common.DispatcherLoopTopEnterEC = AbsoluteLoopTopAddressEnterEC;
    Common.DispatcherLoopTopEnterECFillSRA = AbsoluteLoopTopAddressEnterECFillSRA;
    Common.ExitFunctionLinker = ExitFunctionLinkerAddress;
    Common.ThreadStopHandlerSpillSRA = ThreadStopHandlerAddressSpillSRA;
    Common.ThreadPauseHandlerSpillSRA = ThreadPauseHandlerAddressSpillSRA;
    Common.GuestSignal_SIGILL = GuestSignal_SIGILL;
    Common.GuestSignal_SIGTRAP = GuestSignal_SIGTRAP;
    Common.GuestSignal_SIGSEGV = GuestSignal_SIGSEGV;
    Common.SignalReturnHandler = SignalHandlerReturnAddress;
    Common.SignalReturnHandlerRT = SignalHandlerReturnAddressRT;

    auto& AArch64 = Thread->CurrentFrame->Pointers.AArch64;
    AArch64.LUDIVHandler = LUDIVHandlerAddress;
    AArch64.LDIVHandler = LDIVHandlerAddress;

    // Fill in the fallback handlers
    InterpreterOps::FillFallbackIndexPointers(Common.FallbackHandlerPointers, &ABIPointers[0]);
  }
}

fextl::unique_ptr<Dispatcher> Dispatcher::Create(FEXCore::Context::ContextImpl* CTX) {
  return fextl::make_unique<Dispatcher>(CTX);
}

} // namespace FEXCore::CPU
