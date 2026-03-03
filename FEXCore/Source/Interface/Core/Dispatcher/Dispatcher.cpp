// SPDX-License-Identifier: MIT

#include "Common/VectorRegType.h"
#include "Interface/Context/Context.h"
#include "Interface/Core/CPUBackend.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/LookupCache.h"
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
#include <FEXHeaderUtils/Syscalls.h>

#include <CodeEmitter/Emitter.h>

#ifdef VIXL_SIMULATOR
#include <aarch64/simulator-aarch64.h>
#endif

#include <array>
#include <bit>
#include <cstring>

namespace FEXCore::CPU {

static void SleepThread(FEXCore::Context::ContextImpl* CTX, FEXCore::Core::CpuStateFrame* Frame) {
  CTX->SyscallHandler->SleepThread(CTX, Frame);
}

constexpr size_t MAX_DISPATCHER_CODE_SIZE = FEXCore::Utils::FEX_PAGE_SIZE * 4;

Dispatcher::Dispatcher(FEXCore::Context::ContextImpl* ctx)
  : Arm64Emitter(ctx, FEXCore::Allocator::VirtualAlloc(MAX_DISPATCHER_CODE_SIZE, true), MAX_DISPATCHER_CODE_SIZE)
  , CTX {ctx} {
  EmitDispatcher();

  FEXCore::Allocator::VirtualName("FEXMem_Misc", reinterpret_cast<void*>(GetBufferBase()), MAX_DISPATCHER_CODE_SIZE);
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
  (void)cbnz(ARMEmitter::Size::i32Bit, ENTRY_FILL_SRA_SINGLE_INST_REG, &CompileSingleStep);

  ARMEmitter::BiDirectionalLabel LoopTop {};

#ifdef ARCHITECTURE_arm64ec
  (void)b(&LoopTop);

  AbsoluteLoopTopAddressEnterECFillSRA = GetCursorAddress<uint64_t>();
  ldr(STATE, EC_ENTRY_CPUAREA_REG, CPU_AREA_EMULATOR_DATA_OFFSET);
  FillStaticRegs();

  ldr(RipReg, STATE_PTR(CpuStateFrame, State.rip));
  // Force a single instruction block if ENTRY_FILL_SRA_SINGLE_INST_REG is nonzero entering the JIT, used for inline SMC handling.
  (void)cbnz(ARMEmitter::Size::i32Bit, ENTRY_FILL_SRA_SINGLE_INST_REG, &CompileSingleStep);

  // Enter JIT
  (void)b(&LoopTop);

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
  (void)cbnz(ARMEmitter::Size::i64Bit, TMP1, &LoopTop);

  // If the entry at the TOS is for the target address, pop it and return to the JIT code
  add(ARMEmitter::Size::i64Bit, REG_CALLRET_SP, REG_CALLRET_SP, 0x10);
  ret(TMP2);

  // Enter JIT
#endif

  // We want to ensure that we are 16 byte aligned at the top of this loop
  Align16B();

  (void)Bind(&LoopTop);
  AbsoluteLoopTopAddress = GetCursorAddress<uint64_t>();

  // Load in our RIP
  ldr(RipReg, STATE_PTR(CpuStateFrame, State.rip));

#ifdef ARCHITECTURE_arm64ec
  // Clobbers TMP1/2
  // Check the EC code bitmap incase we need to exit the JIT to call into native code.
  ARMEmitter::ForwardLabel l_NotECCode;
  ldr(TMP1, ARMEmitter::XReg::x18, TEB_PEB_OFFSET);
  ldr(TMP1, TMP1, PEB_EC_CODE_BITMAP_OFFSET);

  lsr(ARMEmitter::Size::i64Bit, TMP2, RipReg, 15);
  and_(ARMEmitter::Size::i64Bit, TMP2, TMP2, 0x1fffffffffff8);
  ldr(TMP1, TMP1, TMP2, ARMEmitter::ExtendedType::LSL_64, 0);
  lsr(ARMEmitter::Size::i64Bit, TMP2, RipReg, 12);
  lsrv(ARMEmitter::Size::i64Bit, TMP1, TMP1, TMP2);
  (void)tbz(TMP1, 0, &l_NotECCode);

  str(REG_CALLRET_SP, STATE_PTR(CpuStateFrame, State.callret_sp));

  add(ARMEmitter::Size::i64Bit, ARMEmitter::Reg::rsp, StaticRegisters[X86State::REG_RSP], 0);
  mov(EC_CALL_CHECKER_PC_REG, RipReg);
  ldr(TMP2, STATE_PTR(CpuStateFrame, Pointers.ExitFunctionEC));
  br(TMP2);

  (void)Bind(&l_NotECCode);
#endif

  ldrb(TMP1, STATE_PTR(CpuStateFrame, State.flags[X86State::RFLAG_TF_RAW_LOC]));
  (void)cbnz(ARMEmitter::Size::i32Bit, TMP1, &CompileSingleStep);

  ARMEmitter::ForwardLabel NoBlock;

  if (DisableL2Cache()) {
    (void)b(&NoBlock);
  } else {
    // This is the block cache lookup routine
    // It matches what is going on it LookupCache.h::FindBlock
    ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.L2Pointer));

    // Mask the address by the virtual address size so we can check for aliases
    uint64_t VirtualMemorySize = CTX->Config.VirtualMemSize;
    if (std::popcount(VirtualMemorySize) == 1) {
      and_(ARMEmitter::Size::i64Bit, TMP4, RipReg.R(), VirtualMemorySize - 1);
    } else {
      LoadConstant(ARMEmitter::Size::i64Bit, TMP4, VirtualMemorySize);
      and_(ARMEmitter::Size::i64Bit, TMP4, RipReg.R(), TMP4);
    }

    {
      // Offset the address and add to our page pointer
      lsr(ARMEmitter::Size::i64Bit, TMP2, TMP4, 12);

      // Load the pointer from the offset
      ldr(TMP1, TMP1, TMP2, ARMEmitter::ExtendedType::LSL_64, 3);

      // If page pointer is zero then we have no block
      (void)cbz(ARMEmitter::Size::i64Bit, TMP1, &NoBlock);

      // Steal the page offset
      and_(ARMEmitter::Size::i64Bit, TMP2, TMP4, 0x0FFF);

      // Shift the offset by the size of the block cache entry
      add(TMP1, TMP1, TMP2, ARMEmitter::ShiftType::LSL, FEXCore::ilog2(sizeof(LookupCache::LookupCacheEntry)));

      // The the full LookupCacheEntry with a single LDP.
      // Check the guest address first to ensure it maps to the address we are currently at.
      // This fixes aliasing problems
      ldp<ARMEmitter::IndexType::OFFSET>(TMP4, TMP2, TMP1, 0);

      // If the guest address doesn't match, Compile the block.
      sub(TMP2, TMP2, RipReg);
      (void)cbnz(ARMEmitter::Size::i64Bit, TMP2, &NoBlock);

      // Check the host address to see if it matches, else compile the block.
      (void)cbz(ARMEmitter::Size::i64Bit, TMP4, &NoBlock);

      // If we've made it here then we have a real compiled block
      {
        // update L1 cache
        ldp<ARMEmitter::IndexType::OFFSET>(TMP1, TMP2, STATE, offsetof(FEXCore::Core::CpuStateFrame, State.L1Pointer));

        // Calculate (tmp1 + ((ripreg & L1_ENTRIES_MASK) << 4)) for the address
        // L1Mask is pre-shifted.
        and_(ARMEmitter::Size::i64Bit, TMP2, TMP2, RipReg.R(), ARMEmitter::ShiftType::LSL, FEXCore::ilog2(sizeof(LookupCache::LookupCacheEntry)));
        add(TMP1, TMP1, TMP2);

        stp<ARMEmitter::IndexType::OFFSET>(TMP4, RipReg, TMP1);

        // Jump to the block
        br(TMP4);
      }
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

#ifdef ARCHITECTURE_arm64ec
    ldr(TMP2, ARMEmitter::XReg::x18, TEB_CPU_AREA_OFFSET);
    LoadConstant(ARMEmitter::Size::i32Bit, TMP1, 1);
    strb(TMP1.W(), TMP2, CPU_AREA_IN_SYSCALL_CALLBACK_OFFSET);
#endif

    Body();

#ifdef ARCHITECTURE_arm64ec
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

      ldr(ARMEmitter::XReg::x2, STATE_PTR(CpuStateFrame, Pointers.ExitFunctionLink));
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

  // Need to create the block
  {
    (void)Bind(&NoBlock);

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
    (void)Bind(&CompileSingleStep);

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
    ldr(ARMEmitter::XReg::x0, STATE_PTR(CpuStateFrame, Pointers.ThunkCallbackRet));

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
    stp<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::zr, ARMEmitter::XReg::zr, REG_CALLRET_SP, -0x10);

    // Now go back to the regular dispatcher loop
    (void)b(&LoopTop);
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

  LUDIVHandlerAddress = EmitLongALUOpHandler(STATE_PTR(CpuStateFrame, Pointers.LUDIV));
  LDIVHandlerAddress = EmitLongALUOpHandler(STATE_PTR(CpuStateFrame, Pointers.LDIV));

  EmitF64Sin();
  EmitF64Cos();
  EmitF64Tan();

  // Interpreter fallbacks
  {
    constexpr static std::array<FallbackABI, FABI_UNKNOWN> ABIS {{
      FABI_F80_I16_F32_PTR,
      FABI_F80_I16_F64_PTR,
      FABI_F80_I16_I16_PTR,
      FABI_F80_I16_I32_PTR,
      FABI_F32_I16_F80_PTR,
      FABI_F64_I16_F80_PTR,
      FABI_F64_F64_PTR,
      FABI_F64_F64_F64_PTR,
      FABI_I16_I16_F80_PTR,
      FABI_I32_I16_F80_PTR,
      FABI_I64_I16_F80_PTR,
      FABI_I64_I16_F80_F80_PTR,
      FABI_F80_I16_F80_PTR,
      FABI_F80_I16_F80_F80_PTR,
      FABI_F80x2_I16_F80_PTR,
      FABI_F64x2_F64_PTR,
      FABI_I32_I64_I64_V128_V128_I16,
      FABI_I32_V128_V128_I16,
    }};

    for (auto ABI : ABIS) {
      ABIPointers[ABI] = GenerateABICall(ABI);
    }
  }

  (void)Bind(&l_CTX);
  dc64(reinterpret_cast<uintptr_t>(CTX));
  (void)Bind(&l_Sleep);
  dc64(reinterpret_cast<uint64_t>(SleepThread));
  (void)Bind(&l_CompileBlock);
  FEXCore::Utils::MemberFunctionToPointerCast PMFCompileBlock(&FEXCore::Context::ContextImpl::CompileBlock);
  dc64(PMFCompileBlock.GetConvertedPointer());
  (void)Bind(&l_CompileSingleStep);

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

void Dispatcher::EmitI32ToExtF80() {
  ARMEmitter::ForwardLabel ZeroCase;
  ARMEmitter::ForwardLabel Done;

  (void)cbz(ARMEmitter::Size::i32Bit, TMP2, &ZeroCase);

  lsr(ARMEmitter::Size::i32Bit, TMP4, TMP2, 31);
  tst(ARMEmitter::Size::i32Bit, TMP2, TMP2);
  neg(ARMEmitter::Size::i32Bit, TMP3, TMP2);
  csel(ARMEmitter::Size::i32Bit, TMP3, TMP3, TMP2, ARMEmitter::Condition::CC_MI);

  clz(ARMEmitter::Size::i32Bit, TMP1, TMP3);

  mov(ARMEmitter::Size::i32Bit, TMP2, 0x401E);
  sub(ARMEmitter::Size::i32Bit, TMP2, TMP2, TMP1);
  orr(ARMEmitter::Size::i32Bit, TMP2, TMP2, TMP4, ARMEmitter::ShiftType::LSL, 15);

  lslv(ARMEmitter::Size::i32Bit, TMP3, TMP3, TMP1);

  lsl(ARMEmitter::Size::i64Bit, TMP3, TMP3, 32);

  fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), TMP3);
  ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 4, TMP2);

  (void)b(&Done);

  (void)Bind(&ZeroCase);
  movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);

  (void)Bind(&Done);
}

void Dispatcher::EmitI16ToExtF80() {
  sxth(ARMEmitter::Size::i32Bit, TMP2, TMP2);

  ARMEmitter::ForwardLabel ZeroCase;
  ARMEmitter::ForwardLabel Done;

  (void)cbz(ARMEmitter::Size::i32Bit, TMP2, &ZeroCase);

  lsr(ARMEmitter::Size::i32Bit, TMP4, TMP2, 31);
  tst(ARMEmitter::Size::i32Bit, TMP2, TMP2);
  neg(ARMEmitter::Size::i32Bit, TMP3, TMP2);
  csel(ARMEmitter::Size::i32Bit, TMP3, TMP3, TMP2, ARMEmitter::Condition::CC_MI);

  clz(ARMEmitter::Size::i32Bit, TMP1, TMP3);

  mov(ARMEmitter::Size::i32Bit, TMP2, 0x401E);
  sub(ARMEmitter::Size::i32Bit, TMP2, TMP2, TMP1);
  orr(ARMEmitter::Size::i32Bit, TMP2, TMP2, TMP4, ARMEmitter::ShiftType::LSL, 15);

  lslv(ARMEmitter::Size::i32Bit, TMP3, TMP3, TMP1);

  lsl(ARMEmitter::Size::i64Bit, TMP3, TMP3, 32);

  fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), TMP3);
  ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 4, TMP2);

  (void)b(&Done);

  (void)Bind(&ZeroCase);
  movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);

  (void)Bind(&Done);
}

void Dispatcher::EmitF32ToExtF80() {
  ARMEmitter::ForwardLabel InfNaN;
  ARMEmitter::ForwardLabel ZeroDenormal;
  ARMEmitter::ForwardLabel Denormal;
  ARMEmitter::ForwardLabel NaN;
  ARMEmitter::ForwardLabel Done;
  ARMEmitter::BiDirectionalLabel NormalPath;
  ARMEmitter::ForwardLabel ZeroResult;

  fmov(ARMEmitter::Size::i32Bit, TMP1, VTMP1.S());

  ubfx(ARMEmitter::Size::i32Bit, TMP2, TMP1, 23, 8);
  and_(ARMEmitter::Size::i32Bit, TMP3, TMP1, 0x007FFFFF);
  lsr(ARMEmitter::Size::i32Bit, TMP4, TMP1, 31);

  cmp(ARMEmitter::Size::i32Bit, TMP2, 0xFF);
  (void)b(ARMEmitter::Condition::CC_EQ, &InfNaN);

  (void)cbz(ARMEmitter::Size::i32Bit, TMP2, &ZeroDenormal);

  (void)Bind(&NormalPath);
  // Exponent bias adjustment, where bias is 0x3F80
  LoadConstant(ARMEmitter::Size::i32Bit, TMP1, 0x3F80);
  add(ARMEmitter::Size::i32Bit, TMP2, TMP2, TMP1);
  orr(ARMEmitter::Size::i32Bit, TMP2, TMP2, TMP4, ARMEmitter::ShiftType::LSL, 15);

  // Set implicit bit and shift fraction to extF80 position
  LoadConstant(ARMEmitter::Size::i64Bit, TMP1, 0x00800000ULL);
  orr(ARMEmitter::Size::i64Bit, TMP3, TMP3, TMP1);
  lsl(ARMEmitter::Size::i64Bit, TMP3, TMP3, 40);

  fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), TMP3);
  ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 4, TMP2);
  (void)b(&Done);

  (void)Bind(&ZeroDenormal);
  (void)cbz(ARMEmitter::Size::i32Bit, TMP3, &ZeroResult);

  (void)Bind(&Denormal);
  clz(ARMEmitter::Size::i32Bit, TMP1, TMP3);
  sub(ARMEmitter::Size::i32Bit, TMP1, TMP1, 8);
  mov(ARMEmitter::Size::i32Bit, TMP2, 1);
  sub(ARMEmitter::Size::i32Bit, TMP2, TMP2, TMP1);
  lslv(ARMEmitter::Size::i32Bit, TMP3, TMP3, TMP1);
  (void)b(&NormalPath);

  (void)Bind(&ZeroResult);
  lsl(ARMEmitter::Size::i32Bit, TMP2, TMP4, 15);
  movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
  ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 4, TMP2);
  (void)b(&Done);

  (void)Bind(&InfNaN);
  (void)cbnz(ARMEmitter::Size::i32Bit, TMP3, &NaN);

  lsl(ARMEmitter::Size::i32Bit, TMP2, TMP4, 15);
  orr(ARMEmitter::Size::i32Bit, TMP2, TMP2, 0x7FFF);

  LoadConstant(ARMEmitter::Size::i64Bit, TMP3, 0x8000000000000000ULL);
  fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), TMP3);
  ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 4, TMP2);
  (void)b(&Done);

  (void)Bind(&NaN);
  lsl(ARMEmitter::Size::i32Bit, TMP2, TMP4, 15);
  orr(ARMEmitter::Size::i32Bit, TMP2, TMP2, 0x7FFF);

  lsl(ARMEmitter::Size::i64Bit, TMP3, TMP3, 40);
  LoadConstant(ARMEmitter::Size::i64Bit, TMP1, 0xC000000000000000ULL);
  orr(ARMEmitter::Size::i64Bit, TMP3, TMP3, TMP1);

  fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), TMP3);
  ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 4, TMP2);

  (void)Bind(&Done);
}

void Dispatcher::EmitF64ToExtF80() {
  ARMEmitter::ForwardLabel InfNaN;
  ARMEmitter::ForwardLabel ZeroDenormal;
  ARMEmitter::ForwardLabel Denormal;
  ARMEmitter::ForwardLabel NaN;
  ARMEmitter::ForwardLabel Done;
  ARMEmitter::BiDirectionalLabel NormalPath;
  ARMEmitter::ForwardLabel ZeroResult;

  fmov(ARMEmitter::Size::i64Bit, TMP1, VTMP1.D());

  lsr(ARMEmitter::Size::i64Bit, TMP4, TMP1, 63);
  ubfx(ARMEmitter::Size::i64Bit, TMP2, TMP1, 52, 11);
  LoadConstant(ARMEmitter::Size::i64Bit, TMP3, 0x000FFFFFFFFFFFFFULL);
  and_(ARMEmitter::Size::i64Bit, TMP3, TMP1, TMP3);

  cmp(ARMEmitter::Size::i64Bit, TMP2, 0x7FF);
  (void)b(ARMEmitter::Condition::CC_EQ, &InfNaN);

  (void)cbz(ARMEmitter::Size::i64Bit, TMP2, &ZeroDenormal);

  (void)Bind(&NormalPath);
  // Exponent bias adjustment where bias difference is 0x3C00
  add(ARMEmitter::Size::i64Bit, TMP2, TMP2, 0x3000);
  add(ARMEmitter::Size::i64Bit, TMP2, TMP2, 0xC00);
  orr(ARMEmitter::Size::i64Bit, TMP2, TMP2, TMP4, ARMEmitter::ShiftType::LSL, 15);

  LoadConstant(ARMEmitter::Size::i64Bit, TMP1, 0x0010000000000000ULL);
  orr(ARMEmitter::Size::i64Bit, TMP3, TMP3, TMP1);
  lsl(ARMEmitter::Size::i64Bit, TMP3, TMP3, 11);

  fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), TMP3);
  ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 4, TMP2);
  (void)b(&Done);

  (void)Bind(&ZeroDenormal);
  (void)cbz(ARMEmitter::Size::i64Bit, TMP3, &ZeroResult);

  (void)Bind(&Denormal);
  clz(ARMEmitter::Size::i64Bit, TMP1, TMP3);
  sub(ARMEmitter::Size::i64Bit, TMP1, TMP1, 11);
  mov(ARMEmitter::Size::i64Bit, TMP2, 1);
  sub(ARMEmitter::Size::i64Bit, TMP2, TMP2, TMP1);
  lslv(ARMEmitter::Size::i64Bit, TMP3, TMP3, TMP1);
  (void)b(&NormalPath);

  (void)Bind(&ZeroResult);
  lsl(ARMEmitter::Size::i64Bit, TMP2, TMP4, 15);
  movi(ARMEmitter::SubRegSize::i64Bit, VTMP1.Q(), 0);
  ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 4, TMP2);
  (void)b(&Done);

  (void)Bind(&InfNaN);
  (void)cbnz(ARMEmitter::Size::i64Bit, TMP3, &NaN);

  lsl(ARMEmitter::Size::i64Bit, TMP2, TMP4, 15);
  orr(ARMEmitter::Size::i64Bit, TMP2, TMP2, 0x7FFF);

  LoadConstant(ARMEmitter::Size::i64Bit, TMP3, 0x8000000000000000ULL);
  fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), TMP3);
  ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 4, TMP2);
  (void)b(&Done);

  (void)Bind(&NaN);
  lsl(ARMEmitter::Size::i64Bit, TMP2, TMP4, 15);
  orr(ARMEmitter::Size::i64Bit, TMP2, TMP2, 0x7FFF);

  lsl(ARMEmitter::Size::i64Bit, TMP3, TMP3, 11);
  LoadConstant(ARMEmitter::Size::i64Bit, TMP1, 0xC000000000000000ULL);
  orr(ARMEmitter::Size::i64Bit, TMP3, TMP3, TMP1);

  fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), TMP3);
  ins(ARMEmitter::SubRegSize::i16Bit, VTMP1, 4, TMP2);

  (void)Bind(&Done);
}

void Dispatcher::EmitF64Sin() {
  F64SinHandlerAddress = GetCursorAddress<uint64_t>();

  constexpr auto V2 = ARMEmitter::VReg::v2;
  constexpr auto V3 = ARMEmitter::VReg::v3;
  constexpr auto V4 = ARMEmitter::VReg::v4;
  constexpr auto V5 = ARMEmitter::VReg::v5;

  ARMEmitter::ForwardLabel Fallback, NonZero;
  ARMEmitter::ForwardLabel InvPiPi1Label, Pi23Label;
  ARMEmitter::ForwardLabel C0Label, C1Label, C2Label, C3Label, C4Label, C5Label, C6Label;
  ARMEmitter::ForwardLabel RangeLabel;

  // sin(+/-0) = +/-0
  fmov(ARMEmitter::Size::i64Bit, TMP1, VTMP1.D());
  lsl(ARMEmitter::Size::i64Bit, TMP1, TMP1, 1);
  (void)cbnz(ARMEmitter::Size::i64Bit, TMP1, &NonZero);
  ret();
  (void)Bind(&NonZero);

  // Save q2-q5.
  stp<ARMEmitter::IndexType::PRE>(ARMEmitter::QReg::q2, ARMEmitter::QReg::q3, ARMEmitter::Reg::rsp, -64);
  stp<ARMEmitter::IndexType::OFFSET>(ARMEmitter::QReg::q4, ARMEmitter::QReg::q5, ARMEmitter::Reg::rsp, 32);

  // save nzcv
  mrs(TMP1, ARMEmitter::SystemRegister::NZCV);
  str(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));

  // Range check: fall back for |x| >= 2^23, NaN, and inf.
  fabs(VTMP2.D(), VTMP1.D());
  ldr(V2.D(), &RangeLabel);
  fcmp(VTMP2.D(), V2.D());
  (void)b(ARMEmitter::Condition::CC_HS, &Fallback);

  // n = rint(x/pi).
  ldr(V2.Q(), &InvPiPi1Label); // q2 = {inv_pi, pi_1}
  fmul(VTMP2.D(), VTMP1.D(), V2.D());
  frinta(VTMP2.D(), VTMP2.D());

  // odd = (int(n) & 1) << 63.
  fcvtzs(ARMEmitter::Size::i64Bit, TMP1, VTMP2.D());
  lsl(ARMEmitter::Size::i64Bit, TMP1, TMP1, 63);

  // r = x - n*pi (range reduction) via .2D lane-indexed FMLS.
  ldr(V3.Q(), &Pi23Label);                                            // q3 = {pi_2, pi_3}
  fmov(V4.D(), VTMP1.D());                                            // r = x
  fmls(ARMEmitter::SubRegSize::i64Bit, V4.Q(), VTMP2.Q(), V2.Q(), 1); // r -= n * pi_1
  fmls(ARMEmitter::SubRegSize::i64Bit, V4.Q(), VTMP2.Q(), V3.Q(), 0); // r -= n * pi_2
  fmls(ARMEmitter::SubRegSize::i64Bit, V4.Q(), VTMP2.Q(), V3.Q(), 1); // r -= n * pi_3

  // r^2, r^4.
  fmul(V5.D(), V4.D(), V4.D());
  fmov(ARMEmitter::Size::i64Bit, TMP2, V4.D());
  fmul(V3.D(), V5.D(), V5.D());

  // Estrin polynomial: p = c0 + r2*c1 + r4*(c2 + r2*c3) + r8*(c4 + r2*c5 + r4*c6).
  // Level 1 (independent FMAs).
  ldr(VTMP1.D(), &C0Label);
  ldr(VTMP2.D(), &C1Label);
  fmadd(VTMP1.D(), V5.D(), VTMP2.D(), VTMP1.D()); // p01 = c0 + r2*c1

  ldr(VTMP2.D(), &C2Label);
  ldr(V2.D(), &C3Label);
  fmadd(VTMP2.D(), V5.D(), V2.D(), VTMP2.D()); // p23 = c2 + r2*c3

  ldr(V2.D(), &C4Label);
  ldr(V4.D(), &C5Label);
  fmadd(V2.D(), V5.D(), V4.D(), V2.D()); // p45 = c4 + r2*c5

  // Level 2 (serial).
  ldr(V4.D(), &C6Label);
  fmadd(V2.D(), V3.D(), V4.D(), V2.D());          // p46 = p45 + r4*c6
  fmadd(VTMP2.D(), V3.D(), V2.D(), VTMP2.D());    // p26 = p23 + r4*p46
  fmadd(VTMP1.D(), V3.D(), VTMP2.D(), VTMP1.D()); // p06 = p01 + r4*p26

  // y = r + r^3 * p06.
  fmov(ARMEmitter::Size::i64Bit, V4.D(), TMP2);
  fmul(V5.D(), V5.D(), V4.D());
  fmadd(VTMP1.D(), V5.D(), VTMP1.D(), V4.D());

  // result = y XOR odd.
  fmov(ARMEmitter::Size::i64Bit, TMP2, VTMP1.D());
  eor(ARMEmitter::Size::i64Bit, TMP2, TMP2, TMP1);
  fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), TMP2);

  // restore nzcv
  ldr(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
  msr(ARMEmitter::SystemRegister::NZCV, TMP1);

  // Restore q2-q5 and return.
  ldp<ARMEmitter::IndexType::OFFSET>(ARMEmitter::QReg::q4, ARMEmitter::QReg::q5, ARMEmitter::Reg::rsp, 32);
  ldp<ARMEmitter::IndexType::POST>(ARMEmitter::QReg::q2, ARMEmitter::QReg::q3, ARMEmitter::Reg::rsp, 64);
  ret();

  // Fallback path.
  (void)Bind(&Fallback);
  ldp<ARMEmitter::IndexType::OFFSET>(ARMEmitter::QReg::q4, ARMEmitter::QReg::q5, ARMEmitter::Reg::rsp, 32);
  ldp<ARMEmitter::IndexType::POST>(ARMEmitter::QReg::q2, ARMEmitter::QReg::q3, ARMEmitter::Reg::rsp, 64);
  str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);
  ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.FallbackHandlerPointers[FEXCore::Core::OPINDEX_F64SIN].ABIHandler));
  ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.FallbackHandlerPointers[FEXCore::Core::OPINDEX_F64SIN].Func));
  blr(TMP1);
  ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
  ret();

  // Constant pool.
  Align(16);
  (void)Bind(&InvPiPi1Label);
  dc64(0x3FD4'5F30'6DC9'C883ULL); // inv_pi
  dc64(0x4009'21FB'5444'2D18ULL); // pi_1
  (void)Bind(&Pi23Label);
  dc64(0x3CA1'A626'3314'5C06ULL); // pi_2
  dc64(0x395C'1CD1'2902'4E09ULL); // pi_3
  (void)Bind(&C0Label);
  dc64(0xBFC5'5555'5555'547BULL); // c0
  (void)Bind(&C1Label);
  dc64(0x3F81'1111'1110'8A4DULL); // c1
  (void)Bind(&C2Label);
  dc64(0xBF2A'01A0'1993'6F27ULL); // c2
  (void)Bind(&C3Label);
  dc64(0x3EC7'1DE3'7A97'D93EULL); // c3
  (void)Bind(&C4Label);
  dc64(0xBE5A'E633'9199'87C6ULL); // c4
  (void)Bind(&C5Label);
  dc64(0x3DE6'0E27'7AE0'7CECULL); // c5
  (void)Bind(&C6Label);
  dc64(0xBD69'E954'0300'A100ULL); // c6
  (void)Bind(&RangeLabel);
  dc64(0x4160'0000'0000'0000ULL); // 2^23
}

void Dispatcher::EmitF64Cos() {
  F64CosHandlerAddress = GetCursorAddress<uint64_t>();

  constexpr auto Accum = ARMEmitter::VReg::v2;

  ARMEmitter::ForwardLabel Fallback;
  ARMEmitter::ForwardLabel RangeLabel, InvPiLabel;
  ARMEmitter::ForwardLabel Pi1Label, Pi2Label, Pi3Label;
  ARMEmitter::ForwardLabel C0Label, C1Label, C2Label, C3Label, C4Label, C5Label, C6Label;

  // Save q2 for use as accumulator
  str<ARMEmitter::IndexType::PRE>(ARMEmitter::QReg::q2, ARMEmitter::Reg::rsp, -16);

  // save nzcv
  mrs(TMP1, ARMEmitter::SystemRegister::NZCV);
  str(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));

  // Range check: fall back for |x| >= 2^23, NaN, and inf.
  fabs(VTMP2.D(), VTMP1.D());
  ldr(Accum.D(), &RangeLabel);
  fcmp(VTMP2.D(), Accum.D());
  (void)b(ARMEmitter::Condition::CC_HS, &Fallback);

  // n = rint(x * (1/pi) + 0.5).
  ldr(Accum.D(), &InvPiLabel);
  fmov(ARMEmitter::ScalarRegSize::i64Bit, VTMP2, 0.5f);
  fmadd(VTMP2.D(), VTMP1.D(), Accum.D(), VTMP2.D());
  frinta(VTMP2.D(), VTMP2.D());

  // odd = (int(n) & 1) << 63.
  fcvtzs(ARMEmitter::Size::i64Bit, TMP1, VTMP2.D());
  lsl(ARMEmitter::Size::i64Bit, TMP1, TMP1, 63);

  // Save input to Accum before overwriting VTMP1.
  fmov(Accum.D(), VTMP1.D());

  // n = n - 0.5.
  fmov(ARMEmitter::ScalarRegSize::i64Bit, VTMP1, 0.5f);
  fsub(VTMP2.D(), VTMP2.D(), VTMP1.D());

  // r = x - n*pi (range reduction), in extended precision.
  ldr(VTMP1.D(), &Pi1Label);
  fmsub(Accum.D(), VTMP2.D(), VTMP1.D(), Accum.D());
  ldr(VTMP1.D(), &Pi2Label);
  fmsub(Accum.D(), VTMP2.D(), VTMP1.D(), Accum.D());
  ldr(VTMP1.D(), &Pi3Label);
  fmsub(Accum.D(), VTMP2.D(), VTMP1.D(), Accum.D());

  // sin(r) poly approx.
  fmul(VTMP1.D(), Accum.D(), Accum.D());
  fmov(ARMEmitter::Size::i64Bit, TMP2, Accum.D());

  // Horner: p = c6 + r2*(c5 + r2*(... + r2*c0)).
  ldr(VTMP2.D(), &C6Label);
  ldr(Accum.D(), &C5Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C4Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C3Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C2Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C1Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C0Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());

  // y = r + r^3 * p.
  fmov(ARMEmitter::Size::i64Bit, Accum.D(), TMP2);
  fmul(VTMP1.D(), VTMP1.D(), Accum.D());
  fmadd(Accum.D(), VTMP1.D(), VTMP2.D(), Accum.D());

  // result = y XOR odd.
  fmov(ARMEmitter::Size::i64Bit, TMP2, Accum.D());
  eor(ARMEmitter::Size::i64Bit, TMP2, TMP2, TMP1);
  fmov(ARMEmitter::Size::i64Bit, VTMP1.D(), TMP2);

  // restore nzcv
  ldr(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
  msr(ARMEmitter::SystemRegister::NZCV, TMP1);

  // Restore q2 and return.
  ldr<ARMEmitter::IndexType::POST>(ARMEmitter::QReg::q2, ARMEmitter::Reg::rsp, 16);
  ret();

  // Fallback path.
  (void)Bind(&Fallback);
  ldr<ARMEmitter::IndexType::POST>(ARMEmitter::QReg::q2, ARMEmitter::Reg::rsp, 16);
  str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);
  ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.FallbackHandlerPointers[FEXCore::Core::OPINDEX_F64COS].ABIHandler));
  ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.FallbackHandlerPointers[FEXCore::Core::OPINDEX_F64COS].Func));
  blr(TMP1);
  ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
  ret();

  // Constant pool.
  Align(16);
  (void)Bind(&InvPiLabel);
  dc64(0x3FD4'5F30'6DC9'C883ULL); // inv_pi
  (void)Bind(&Pi1Label);
  dc64(0x4009'21FB'5444'2D18ULL); // pi_1
  (void)Bind(&Pi2Label);
  dc64(0x3CA1'A626'3314'5C06ULL); // pi_2
  (void)Bind(&Pi3Label);
  dc64(0x395C'1CD1'2902'4E09ULL); // pi_3
  (void)Bind(&C0Label);
  dc64(0xBFC5'5555'5555'547BULL); // c0
  (void)Bind(&C1Label);
  dc64(0x3F81'1111'1110'8A4DULL); // c1
  (void)Bind(&C2Label);
  dc64(0xBF2A'01A0'1993'6F27ULL); // c2
  (void)Bind(&C3Label);
  dc64(0x3EC7'1DE3'7A97'D93EULL); // c3
  (void)Bind(&C4Label);
  dc64(0xBE5A'E633'9199'87C6ULL); // c4
  (void)Bind(&C5Label);
  dc64(0x3DE6'0E27'7AE0'7CECULL); // c5
  (void)Bind(&C6Label);
  dc64(0xBD69'E954'0300'A100ULL); // c6
  (void)Bind(&RangeLabel);
  dc64(0x4160'0000'0000'0000ULL); // 2^23
}

void Dispatcher::EmitF64Tan() {
  F64TanHandlerAddress = GetCursorAddress<uint64_t>();

  constexpr auto Accum = ARMEmitter::VReg::v2;

  ARMEmitter::ForwardLabel Fallback, NonZero;
  ARMEmitter::ForwardLabel RangeLabel, TwoOverPiLabel;
  ARMEmitter::ForwardLabel HalfPi0Label, HalfPi1Label;
  ARMEmitter::ForwardLabel C0Label, C1Label, C2Label, C3Label, C4Label, C5Label, C6Label, C7Label, C8Label;

  // tan(+/-0) = +/-0
  fmov(ARMEmitter::Size::i64Bit, TMP1, VTMP1.D());
  lsl(ARMEmitter::Size::i64Bit, TMP1, TMP1, 1);
  (void)cbnz(ARMEmitter::Size::i64Bit, TMP1, &NonZero);
  ret();
  (void)Bind(&NonZero);

  // Save q2 for use as accumulator
  str<ARMEmitter::IndexType::PRE>(ARMEmitter::QReg::q2, ARMEmitter::Reg::rsp, -16);

  // save nzcv
  mrs(TMP1, ARMEmitter::SystemRegister::NZCV);
  str(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));

  // Range check: fall back for |x| >= 2^23, NaN, and inf.
  fabs(VTMP2.D(), VTMP1.D());
  ldr(Accum.D(), &RangeLabel);
  fcmp(VTMP2.D(), Accum.D());
  (void)b(ARMEmitter::Condition::CC_HS, &Fallback);

  // q = nearest integer to 2 * x / pi.
  ldr(VTMP2.D(), &TwoOverPiLabel);
  fmul(VTMP2.D(), VTMP1.D(), VTMP2.D());
  frinta(VTMP2.D(), VTMP2.D());

  // qi = int(q).
  fcvtzs(ARMEmitter::Size::i64Bit, TMP1, VTMP2.D());

  // r = x - q * pi/2 (range reduction), in extended precision.
  fmov(Accum.D(), VTMP1.D());
  ldr(VTMP1.D(), &HalfPi0Label);
  fmsub(Accum.D(), VTMP2.D(), VTMP1.D(), Accum.D());
  ldr(VTMP1.D(), &HalfPi1Label);
  fmsub(Accum.D(), VTMP2.D(), VTMP1.D(), Accum.D());

  // Further reduce r to [-pi/8, pi/8].
  fmov(ARMEmitter::ScalarRegSize::i64Bit, VTMP1, 0.5f);
  fmul(Accum.D(), Accum.D(), VTMP1.D());

  // Approximate tan(r) using order 8 polynomial.
  fmul(VTMP1.D(), Accum.D(), Accum.D());
  fmov(ARMEmitter::Size::i64Bit, TMP2, Accum.D());

  // Horner: p = C8 + r2*(C7 + r2*(... + r2*C0)).
  ldr(VTMP2.D(), &C8Label);
  ldr(Accum.D(), &C7Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C6Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C5Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C4Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C3Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C2Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C1Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());
  ldr(Accum.D(), &C0Label);
  fmadd(VTMP2.D(), VTMP1.D(), VTMP2.D(), Accum.D());

  // p = r + r^3 * p.
  fmov(ARMEmitter::Size::i64Bit, Accum.D(), TMP2);
  fmul(VTMP1.D(), VTMP1.D(), Accum.D());
  fmadd(Accum.D(), VTMP1.D(), VTMP2.D(), Accum.D());

  // Double-angle reconstruction: tan(2x) = 2*tan(x) / (1 - tan^2(x)).
  fadd(VTMP1.D(), Accum.D(), Accum.D());
  fmul(VTMP2.D(), Accum.D(), Accum.D());
  fmov(ARMEmitter::ScalarRegSize::i64Bit, Accum, 1.0f);
  fsub(VTMP2.D(), VTMP2.D(), Accum.D());

  ARMEmitter::ForwardLabel SkipSwap;
  (void)tbnz(TMP1, 0, &SkipSwap);

  fneg(Accum.D(), VTMP1.D());
  fmov(VTMP1.D(), VTMP2.D());
  fmov(VTMP2.D(), Accum.D());

  (void)Bind(&SkipSwap);

  // result = numerator / denominator -> VTMP1.
  fdiv(VTMP1.D(), VTMP2.D(), VTMP1.D());

  // restore nzcv
  ldr(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
  msr(ARMEmitter::SystemRegister::NZCV, TMP1);

  // Restore q2 and return.
  ldr<ARMEmitter::IndexType::POST>(ARMEmitter::QReg::q2, ARMEmitter::Reg::rsp, 16);
  ret();

  // Fallback path.
  (void)Bind(&Fallback);
  ldr<ARMEmitter::IndexType::POST>(ARMEmitter::QReg::q2, ARMEmitter::Reg::rsp, 16);
  str<ARMEmitter::IndexType::PRE>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, -16);
  ldr(TMP1, STATE_PTR(CpuStateFrame, Pointers.FallbackHandlerPointers[FEXCore::Core::OPINDEX_F64TAN].ABIHandler));
  ldr(TMP4, STATE_PTR(CpuStateFrame, Pointers.FallbackHandlerPointers[FEXCore::Core::OPINDEX_F64TAN].Func));
  blr(TMP1);
  ldr<ARMEmitter::IndexType::POST>(ARMEmitter::XReg::lr, ARMEmitter::Reg::rsp, 16);
  ret();

  // Constant pool.
  Align(16);
  (void)Bind(&TwoOverPiLabel);
  dc64(0x3FE4'5F30'6DC9'C883ULL); // two_over_pi
  (void)Bind(&HalfPi0Label);
  dc64(0x3FF9'21FB'5444'2D18ULL); // half_pi[0]
  (void)Bind(&HalfPi1Label);
  dc64(0x3C91'A626'3314'5C07ULL); // half_pi[1]
  (void)Bind(&C0Label);
  dc64(0x3FD5'5555'5555'5556ULL); // C0
  (void)Bind(&C1Label);
  dc64(0x3FC1'1111'1111'0A63ULL); // C1
  (void)Bind(&C2Label);
  dc64(0x3FAB'A1BA'1BB4'6414ULL); // C2
  (void)Bind(&C3Label);
  dc64(0x3F96'64F4'7E5B'5445ULL); // C3
  (void)Bind(&C4Label);
  dc64(0x3F82'26E5'E5EC'DFA3ULL); // C4
  (void)Bind(&C5Label);
  dc64(0x3F6D'6C7D'DBF8'7047ULL); // C5
  (void)Bind(&C6Label);
  dc64(0x3F57'EA75'D05B'583EULL); // C6
  (void)Bind(&C7Label);
  dc64(0x3F42'89F2'2964'A03CULL); // C7
  (void)Bind(&C8Label);
  dc64(0x3F34'E4FD'1414'7622ULL); // C8
  (void)Bind(&RangeLabel);
  dc64(0x4160'0000'0000'0000ULL); // 2^23
}

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
    // Save NZCV - it's a static register (guest x86 flags) and the inline code clobbers it
    mrs(TMP1, ARMEmitter::SystemRegister::NZCV);
    str(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
    EmitF32ToExtF80();
    ldr(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
    msr(ARMEmitter::SystemRegister::NZCV, TMP1);
  } break;
  case FABI_F80_I16_F64_PTR: {
    mrs(TMP1, ARMEmitter::SystemRegister::NZCV);
    str(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
    EmitF64ToExtF80();
    ldr(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
    msr(ARMEmitter::SystemRegister::NZCV, TMP1);
  } break;
  case FABI_F80_I16_I16_PTR: {
    mrs(TMP1, ARMEmitter::SystemRegister::NZCV);
    str(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
    EmitI16ToExtF80();
    ldr(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
    msr(ARMEmitter::SystemRegister::NZCV, TMP1);
  } break;
  case FABI_F80_I16_I32_PTR: {
    mrs(TMP1, ARMEmitter::SystemRegister::NZCV);
    str(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
    EmitI32ToExtF80();
    ldr(TMP1.W(), STATE.R(), offsetof(FEXCore::Core::CpuStateFrame, State.flags[24]));
    msr(ARMEmitter::SystemRegister::NZCV, TMP1);
  } break;
  case FABI_F32_I16_F80_PTR: {
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
  case FABI_F64_F64_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): vector source
    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    if (!TMP_ABIARGS) {
      fmov(VABI1.D(), VTMP1.D());
    }
    mov(ARMEmitter::XReg::x0, STATE);

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<double, double, uint64_t>(FallbackPointerReg);
    } else {
      blr(FallbackPointerReg);
    }

    FillF64Result();
  } break;
  case FABI_F64_F64_F64_PTR: {
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

    mov(ARMEmitter::XReg::x0, STATE);
    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      GenerateIndirectRuntimeCall<double, double, double, uint64_t>(FallbackPointerReg);
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
  case FABI_F64x2_F64_PTR: {
    // Linux Reg/Win32 Reg:
    // tmp4 (x4/x13): FallbackHandler
    // x30: return
    // vtmp1 (v0/v16): vector source 1
    // vtmp2 (v1/v16): vector source 2

    SpillForABICall(CTX->HostFeatures.SupportsPreserveAllABI, TMP3, true);

    mov(ARMEmitter::XReg::x0, STATE);
    if (!TMP_ABIARGS) {
      fmov(VABI1.D(), VTMP1.D());
    }

    if (!CTX->Config.DisableVixlIndirectCalls) [[unlikely]] {
      // GenerateIndirectRuntimeCall<FEXCore::VectorScalarF64Pair, FEXCore::VectorRegType, uint64_t>(FallbackPointerReg);
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
    auto& Ptrs = Thread->CurrentFrame->Pointers;

    Ptrs.DispatcherLoopTop = AbsoluteLoopTopAddress;
    Ptrs.DispatcherLoopTopFillSRA = AbsoluteLoopTopAddressFillSRA;
    Ptrs.DispatcherLoopTopEnterEC = AbsoluteLoopTopAddressEnterEC;
    Ptrs.DispatcherLoopTopEnterECFillSRA = AbsoluteLoopTopAddressEnterECFillSRA;
    Ptrs.ExitFunctionLinker = ExitFunctionLinkerAddress;
    Ptrs.ThreadStopHandlerSpillSRA = ThreadStopHandlerAddressSpillSRA;
    Ptrs.ThreadPauseHandlerSpillSRA = ThreadPauseHandlerAddressSpillSRA;
    Ptrs.GuestSignal_SIGILL = GuestSignal_SIGILL;
    Ptrs.GuestSignal_SIGTRAP = GuestSignal_SIGTRAP;
    Ptrs.GuestSignal_SIGSEGV = GuestSignal_SIGSEGV;
    Ptrs.SignalReturnHandler = SignalHandlerReturnAddress;
    Ptrs.SignalReturnHandlerRT = SignalHandlerReturnAddressRT;
    Ptrs.LUDIVHandler = LUDIVHandlerAddress;
    Ptrs.LDIVHandler = LDIVHandlerAddress;
    Ptrs.F64SinHandler = F64SinHandlerAddress;
    Ptrs.F64CosHandler = F64CosHandlerAddress;
    Ptrs.F64TanHandler = F64TanHandlerAddress;

    // Fill in the fallback handlers
    InterpreterOps::FillFallbackIndexPointers(Ptrs.FallbackHandlerPointers, &ABIPointers[0]);
  }
}

SignalDelegatorConfig Dispatcher::MakeSignalDelegatorConfig() const {
  // PF/AF are the final two SRA registers. We only want GPRs
  const auto GPRCount = uint16_t(StaticRegisters.size() - 2);
  const auto FPRCount = uint16_t(StaticFPRegisters.size());

  const auto GetSRAGPRMapping = [GPRCount, this] {
    SignalDelegatorConfig::SRAIndexMapping Mapping {};
    for (size_t i = 0; i < GPRCount; ++i) {
      Mapping[i] = StaticRegisters[i].Idx();
    }
    return Mapping;
  };

  const auto GetSRAFPRMapping = [FPRCount, this] {
    SignalDelegatorConfig::SRAIndexMapping Mapping {};
    for (size_t i = 0; i < FPRCount; ++i) {
      Mapping[i] = StaticFPRegisters[i].Idx();
    }
    return Mapping;
  };

  return FEXCore::SignalDelegatorConfig {
    .DispatcherBegin = Start,
    .DispatcherEnd = End,

    .AbsoluteLoopTopAddress = AbsoluteLoopTopAddress,
    .AbsoluteLoopTopAddressFillSRA = AbsoluteLoopTopAddressFillSRA,
    .SignalHandlerReturnAddress = SignalHandlerReturnAddress,
    .SignalHandlerReturnAddressRT = SignalHandlerReturnAddressRT,

    .PauseReturnInstruction = PauseReturnInstruction,
    .ThreadPauseHandlerAddressSpillSRA = ThreadPauseHandlerAddressSpillSRA,
    .ThreadPauseHandlerAddress = ThreadPauseHandlerAddress,

    // Stop handlers.
    .ThreadStopHandlerAddressSpillSRA = ThreadStopHandlerAddressSpillSRA,
    .ThreadStopHandlerAddress = ThreadStopHandlerAddress,

    // SRA information.
    .SRAGPRCount = GPRCount,
    .SRAFPRCount = FPRCount,

    .SRAGPRMapping = GetSRAGPRMapping(),
    .SRAFPRMapping = GetSRAFPRMapping(),
  };
}

fextl::unique_ptr<Dispatcher> Dispatcher::Create(FEXCore::Context::ContextImpl* CTX) {
  return fextl::make_unique<Dispatcher>(CTX);
}

} // namespace FEXCore::CPU
