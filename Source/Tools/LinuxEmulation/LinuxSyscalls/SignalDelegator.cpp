// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|common
desc: Handles host -> host and host -> guest signal routing, emulates procmask & co
$end_info$
*/

#include "LinuxSyscalls/SignalDelegator.h"
#include "LinuxSyscalls/Syscalls.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/FPState.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <atomic>
#include <string.h>

#include <errno.h>
#include <exception>
#include <functional>
#include <linux/futex.h>
#include <signal.h>
#include <syscall.h>
#include <sys/mman.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <utility>

// For older build environments
#ifndef SS_AUTODISARM
#define SS_AUTODISARM (1U << 31)
#endif

namespace FEX::HLE {
#ifdef _M_X86_64
__attribute__((naked)) static void sigrestore() {
  __asm volatile("syscall;" ::"a"(0xF) : "memory");
}
#endif

constexpr static uint32_t X86_MINSIGSTKSZ = 0x2000U;

// We can only have one delegator per process
static SignalDelegator* GlobalDelegator {};

struct ThreadState {
  FEX::HLE::ThreadStateObject* Thread {};

  void* AltStackPtr {};
  stack_t GuestAltStack {
    .ss_sp = nullptr,
    .ss_flags = SS_DISABLE, // By default the guest alt stack is disabled
    .ss_size = 0,
  };
  // This is the thread's current signal mask
  GuestSAMask CurrentSignalMask {};
  // The mask prior to a suspend
  GuestSAMask PreviousSuspendMask {};

  uint64_t PendingSignals {};
};

thread_local ThreadState ThreadData {};

static void SignalHandlerThunk(int Signal, siginfo_t* Info, void* UContext) {
  GlobalDelegator->HandleSignal(Signal, Info, UContext);
}

uint64_t SigIsMember(GuestSAMask* Set, int Signal) {
  // Signal 0 isn't real, so everything is offset by one inside the set
  Signal -= 1;
  return (Set->Val >> Signal) & 1;
}

uint64_t SetSignal(GuestSAMask* Set, int Signal) {
  // Signal 0 isn't real, so everything is offset by one inside the set
  Signal -= 1;
  return Set->Val | (1ULL << Signal);
}

/**
 * @name Signal frame setup
 * @{ */

// Total number of layouts that siginfo supports.
enum class SigInfoLayout {
  LAYOUT_KILL,
  LAYOUT_TIMER,
  LAYOUT_POLL,
  LAYOUT_FAULT,
  LAYOUT_FAULT_RIP,
  LAYOUT_CHLD,
  LAYOUT_RT,
  LAYOUT_SYS,
};

// Calculate the siginfo layout based on Signal and si_code.
static SigInfoLayout CalculateSigInfoLayout(int Signal, int si_code) {
  if (si_code > SI_USER && si_code < SI_KERNEL) {
    // For signals that are not considered RT.
    if (Signal == SIGSEGV || Signal == SIGBUS || Signal == SIGTRAP) {
      // Regular FAULT layout.
      return SigInfoLayout::LAYOUT_FAULT;
    } else if (Signal == SIGILL || Signal == SIGFPE) {
      // Fault layout but addr refers to RIP.
      return SigInfoLayout::LAYOUT_FAULT_RIP;
    } else if (Signal == SIGCHLD) {
      // Child layout
      return SigInfoLayout::LAYOUT_CHLD;
    } else if (Signal == SIGPOLL) {
      // Poll layout
      return SigInfoLayout::LAYOUT_POLL;
    } else if (Signal == SIGSYS) {
      // Sys layout
      return SigInfoLayout::LAYOUT_SYS;
    }
  } else {
    // Negative si_codes are kernel specific things.
    if (si_code == SI_TIMER) {
      return SigInfoLayout::LAYOUT_TIMER;
    } else if (si_code == SI_SIGIO) {
      return SigInfoLayout::LAYOUT_POLL;
    } else if (si_code < 0) {
      return SigInfoLayout::LAYOUT_RT;
    }
  }

  return SigInfoLayout::LAYOUT_KILL;
}

void SignalDelegator::HandleSignal(int Signal, void* Info, void* UContext) {
  // Let the host take first stab at handling the signal
  auto Thread = GetTLSThread();

  if (!Thread) {
    LogMan::Msg::AFmt("[{}] Thread has received a signal and hasn't registered itself with the delegate! Programming error!",
                      FHU::Syscalls::gettid());
  } else {
    SignalHandler& Handler = HostHandlers[Signal];
    for (auto& HandlerFunc : Handler.Handlers) {
      if (HandlerFunc(Thread->Thread, Signal, Info, UContext)) {
        // If the host handler handled the fault then we can continue now
        return;
      }
    }

    if (Handler.FrontendHandler && Handler.FrontendHandler(Thread->Thread, Signal, Info, UContext)) {
      return;
    }

    // Now let the frontend handle the signal
    // It's clearly a guest signal and this ends up being an OS specific issue
    HandleGuestSignal(Thread->Thread, Signal, Info, UContext);
  }
}

void SignalDelegator::RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
  SetHostSignalHandler(Signal, Func, Required);
  FrontendRegisterHostSignalHandler(Signal, Func, Required);
}

void SignalDelegator::SpillSRA(FEXCore::Core::InternalThreadState* Thread, void* ucontext, uint32_t IgnoreMask) {
#ifdef _M_ARM_64
  for (size_t i = 0; i < Config.SRAGPRCount; i++) {
    const uint8_t SRAIdxMap = Config.SRAGPRMapping[i];
    if (IgnoreMask & (1U << SRAIdxMap)) {
      // Skip this one, it's already spilled
      continue;
    }
    Thread->CurrentFrame->State.gregs[i] = ArchHelpers::Context::GetArmReg(ucontext, SRAIdxMap);
  }

  if (Config.SupportsAVX) {
    // TODO: This doesn't save the upper 128-bits of the 256-bit registers.
    // This needs to be implemented still.
    for (size_t i = 0; i < Config.SRAFPRCount; i++) {
      auto FPR = ArchHelpers::Context::GetArmFPR(ucontext, Config.SRAFPRMapping[i]);
      memcpy(&Thread->CurrentFrame->State.xmm.avx.data[i][0], &FPR, sizeof(__uint128_t));
    }
  } else {
    for (size_t i = 0; i < Config.SRAFPRCount; i++) {
      auto FPR = ArchHelpers::Context::GetArmFPR(ucontext, Config.SRAFPRMapping[i]);
      memcpy(&Thread->CurrentFrame->State.xmm.sse.data[i][0], &FPR, sizeof(__uint128_t));
    }
  }
#endif
}

static uint32_t ConvertSignalToTrapNo(int Signal, siginfo_t* HostSigInfo) {
  switch (Signal) {
  case SIGSEGV:
    if (HostSigInfo->si_code == SEGV_MAPERR || HostSigInfo->si_code == SEGV_ACCERR) {
      // Protection fault
      return FEXCore::X86State::X86_TRAPNO_PF;
    }
    break;
  }

  // Unknown mapping, fall back to old behaviour and just pass signal
  return Signal;
}

static uint32_t ConvertSignalToError(void* ucontext, int Signal, siginfo_t* HostSigInfo) {
  switch (Signal) {
  case SIGSEGV:
    if (HostSigInfo->si_code == SEGV_MAPERR || HostSigInfo->si_code == SEGV_ACCERR) {
      // Protection fault
      // Always a user fault for us
      return ArchHelpers::Context::GetProtectFlags(ucontext);
    }
    break;
  }

  // Not a page fault issue
  return 0;
}

template<typename T>
static void SetXStateInfo(T* xstate, bool is_avx_enabled) {
  auto* fpstate = &xstate->fpstate;

  fpstate->sw_reserved.magic1 = FEXCore::x86_64::fpx_sw_bytes::FP_XSTATE_MAGIC;
  fpstate->sw_reserved.extended_size = is_avx_enabled ? sizeof(T) : 0;

  fpstate->sw_reserved.xfeatures |= FEXCore::x86_64::fpx_sw_bytes::FEATURE_FP | FEXCore::x86_64::fpx_sw_bytes::FEATURE_SSE;
  if (is_avx_enabled) {
    fpstate->sw_reserved.xfeatures |= FEXCore::x86_64::fpx_sw_bytes::FEATURE_YMM;
  }

  fpstate->sw_reserved.xstate_size = fpstate->sw_reserved.extended_size;

  if (is_avx_enabled) {
    xstate->xstate_hdr.xfeatures = 0;
  }
}

ArchHelpers::Context::ContextBackup* SignalDelegator::StoreThreadState(FEXCore::Core::InternalThreadState* Thread, int Signal, void* ucontext) {
  // We can end up getting a signal at any point in our host state
  // Jump to a handler that saves all state so we can safely return
  uint64_t OldSP = ArchHelpers::Context::GetSp(ucontext);
  uintptr_t NewSP = OldSP;

  size_t StackOffset = sizeof(ArchHelpers::Context::ContextBackup);

  // We need to back up behind the host's red zone
  // We do this on the guest side as well
  // (does nothing on arm hosts)
  NewSP -= ArchHelpers::Context::ContextBackup::RedZoneSize;

  NewSP -= StackOffset;
  NewSP = FEXCore::AlignDown(NewSP, 16);

  auto Context = reinterpret_cast<ArchHelpers::Context::ContextBackup*>(NewSP);
  ArchHelpers::Context::BackupContext(ucontext, Context);

  // Retain the action pointer so we can see it when we return
  Context->Signal = Signal;

  // Save guest state
  // We can't guarantee if registers are in context or host GPRs
  // So we need to save everything
  memcpy(&Context->GuestState, &Thread->CurrentFrame->State, sizeof(FEXCore::Core::CPUState));

  // Set the new SP
  ArchHelpers::Context::SetSp(ucontext, NewSP);

  Context->Flags = 0;
  Context->FPStateLocation = 0;
  Context->UContextLocation = 0;
  Context->SigInfoLocation = 0;
  Context->InSyscallInfo = 0;

  // Store fault to top status and then reset it
  Context->FaultToTopAndGeneratedException = Thread->CurrentFrame->SynchronousFaultData.FaultToTopAndGeneratedException;
  Thread->CurrentFrame->SynchronousFaultData.FaultToTopAndGeneratedException = false;

  return Context;
}

void SignalDelegator::RestoreThreadState(FEXCore::Core::InternalThreadState* Thread, void* ucontext, RestoreType Type) {
  const bool IsAVXEnabled = Config.SupportsAVX;

  uint64_t OldSP {};
  if (Type == RestoreType::TYPE_PAUSE) [[unlikely]] {
    OldSP = ArchHelpers::Context::GetSp(ucontext);
  } else {
    // Some fun introspection here.
    // We store a pointer to our host-stack on the guest stack.
    // We need to inspect the guest state coming in, so we can get our host stack back.
    uint64_t GuestSP = Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RSP];

    if (Is64BitMode) {
      // Signal frame layout on stack needs to be as follows
      // void* ReturnPointer
      // ucontext_t
      // siginfo_t
      // FP state
      // Host stack location

      GuestSP += sizeof(FEXCore::x86_64::ucontext_t);
      GuestSP = FEXCore::AlignUp(GuestSP, alignof(FEXCore::x86_64::ucontext_t));

      GuestSP += sizeof(siginfo_t);
      GuestSP = FEXCore::AlignUp(GuestSP, alignof(siginfo_t));

      if (IsAVXEnabled) {
        GuestSP += sizeof(FEXCore::x86_64::xstate);
        GuestSP = FEXCore::AlignUp(GuestSP, alignof(FEXCore::x86_64::xstate));
      } else {
        GuestSP += sizeof(FEXCore::x86_64::_libc_fpstate);
        GuestSP = FEXCore::AlignUp(GuestSP, alignof(FEXCore::x86_64::_libc_fpstate));
      }
    } else {
      if (Type == RestoreType::TYPE_NONREALTIME) {
        // Signal frame layout on stack needs to be as follows
        // SigFrame_i32
        // FPState
        // Host stack location

        // Remove the 4-byte pretcode /AND/ a legacy argument that is ignored.
        GuestSP += sizeof(SigFrame_i32) - 8;
        GuestSP = FEXCore::AlignUp(GuestSP, alignof(SigFrame_i32));

        if (IsAVXEnabled) {
          GuestSP += sizeof(FEXCore::x86::xstate);
          GuestSP = FEXCore::AlignUp(GuestSP, alignof(FEXCore::x86::xstate));
        } else {
          GuestSP += sizeof(FEXCore::x86::_libc_fpstate);
          GuestSP = FEXCore::AlignUp(GuestSP, alignof(FEXCore::x86::_libc_fpstate));
        }
      } else {
        // Signal frame layout on stack needs to be as follows
        // RTSigFrame_i32
        // FPState
        // Host stack location

        // Remove the 4-byte pretcode.
        GuestSP += sizeof(RTSigFrame_i32) - 4;
        GuestSP = FEXCore::AlignUp(GuestSP, alignof(RTSigFrame_i32));

        if (IsAVXEnabled) {
          GuestSP += sizeof(FEXCore::x86::xstate);
          GuestSP = FEXCore::AlignUp(GuestSP, alignof(FEXCore::x86::xstate));
        } else {
          GuestSP += sizeof(FEXCore::x86::_libc_fpstate);
          GuestSP = FEXCore::AlignUp(GuestSP, alignof(FEXCore::x86::_libc_fpstate));
        }
      }
    }

    OldSP = *reinterpret_cast<uint64_t*>(GuestSP);
  }

  uintptr_t NewSP = OldSP;
  auto Context = reinterpret_cast<ArchHelpers::Context::ContextBackup*>(NewSP);

  // Restore host state
  ArchHelpers::Context::RestoreContext(ucontext, Context);

  // Reset the guest state
  memcpy(&Thread->CurrentFrame->State, &Context->GuestState, sizeof(FEXCore::Core::CPUState));

  if (Context->UContextLocation) {
    auto Frame = Thread->CurrentFrame;

    if (Context->Flags & ArchHelpers::Context::ContextFlags::CONTEXT_FLAG_INJIT) {
      // XXX: Unsupported since it needs state reconstruction
      // If we are in the JIT then SRA might need to be restored to values from the context
      // We can't currently support this since it might result in tearing without real state reconstruction
    }

    if (Is64BitMode) {
      RestoreFrame_x64(Thread, Context, Frame, ucontext);
    } else {
      if (Type == RestoreType::TYPE_NONREALTIME) {
        RestoreFrame_ia32(Thread, Context, Frame, ucontext);
      } else {
        RestoreRTFrame_ia32(Thread, Context, Frame, ucontext);
      }
    }
  }
}

void SignalDelegator::RestoreFrame_x64(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* Context,
                                       FEXCore::Core::CpuStateFrame* Frame, void* ucontext) {
  const bool IsAVXEnabled = Config.SupportsAVX;

  auto* guest_uctx = reinterpret_cast<FEXCore::x86_64::ucontext_t*>(Context->UContextLocation);
  [[maybe_unused]] auto* guest_siginfo = reinterpret_cast<siginfo_t*>(Context->SigInfoLocation);

  // If the guest modified the RIP then we need to take special precautions here
  if (Context->OriginalRIP != guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RIP] || Context->FaultToTopAndGeneratedException) {

    // Restore previous `InSyscallInfo` structure.
    Frame->InSyscallInfo = Context->InSyscallInfo;

    // Hack! Go back to the top of the dispatcher top
    // This is only safe inside the JIT rather than anything outside of it
    ArchHelpers::Context::SetPc(ucontext, Config.AbsoluteLoopTopAddressFillSRA);
    // Set our state register to point to our guest thread data
    ArchHelpers::Context::SetState(ucontext, reinterpret_cast<uint64_t>(Frame));

    Frame->State.rip = guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RIP];
    // XXX: Full context setting
    CTX->SetFlagsFromCompactedEFLAGS(Thread, guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_EFL]);

#define COPY_REG(x) Frame->State.gregs[FEXCore::X86State::REG_##x] = guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_##x];
    COPY_REG(R8);
    COPY_REG(R9);
    COPY_REG(R10);
    COPY_REG(R11);
    COPY_REG(R12);
    COPY_REG(R13);
    COPY_REG(R14);
    COPY_REG(R15);
    COPY_REG(RDI);
    COPY_REG(RSI);
    COPY_REG(RBP);
    COPY_REG(RBX);
    COPY_REG(RDX);
    COPY_REG(RAX);
    COPY_REG(RCX);
    COPY_REG(RSP);
#undef COPY_REG
    auto* xstate = reinterpret_cast<FEXCore::x86_64::xstate*>(guest_uctx->uc_mcontext.fpregs);
    auto* fpstate = &xstate->fpstate;

    // Copy float registers
    memcpy(Frame->State.mm, fpstate->_st, sizeof(Frame->State.mm));

    if (IsAVXEnabled) {
      CTX->SetXMMRegistersFromState(Thread, fpstate->_xmm, xstate->ymmh.ymmh_space);
    } else {
      CTX->SetXMMRegistersFromState(Thread, fpstate->_xmm, nullptr);
    }

    // FCW store default
    Frame->State.FCW = fpstate->fcw;
    Frame->State.AbridgedFTW = fpstate->ftw;

    // Deconstruct FSW
    Frame->State.flags[FEXCore::X86State::X87FLAG_C0_LOC] = (fpstate->fsw >> 8) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_C1_LOC] = (fpstate->fsw >> 9) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_C2_LOC] = (fpstate->fsw >> 10) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_C3_LOC] = (fpstate->fsw >> 14) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] = (fpstate->fsw >> 11) & 0b111;
  }
}

void SignalDelegator::RestoreFrame_ia32(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* Context,
                                        FEXCore::Core::CpuStateFrame* Frame, void* ucontext) {
  const bool IsAVXEnabled = Config.SupportsAVX;

  SigFrame_i32* guest_uctx = reinterpret_cast<SigFrame_i32*>(Context->UContextLocation);
  // If the guest modified the RIP then we need to take special precautions here
  if (Context->OriginalRIP != guest_uctx->sc.ip || Context->FaultToTopAndGeneratedException) {
    // Restore previous `InSyscallInfo` structure.
    Frame->InSyscallInfo = Context->InSyscallInfo;

    // Hack! Go back to the top of the dispatcher top
    // This is only safe inside the JIT rather than anything outside of it
    ArchHelpers::Context::SetPc(ucontext, Config.AbsoluteLoopTopAddressFillSRA);
    // Set our state register to point to our guest thread data
    ArchHelpers::Context::SetState(ucontext, reinterpret_cast<uint64_t>(Frame));

    // XXX: Full context setting
    CTX->SetFlagsFromCompactedEFLAGS(Thread, guest_uctx->sc.flags);

    Frame->State.rip = guest_uctx->sc.ip;
    Frame->State.cs_idx = guest_uctx->sc.cs;
    Frame->State.ds_idx = guest_uctx->sc.ds;
    Frame->State.es_idx = guest_uctx->sc.es;
    Frame->State.fs_idx = guest_uctx->sc.fs;
    Frame->State.gs_idx = guest_uctx->sc.gs;
    Frame->State.ss_idx = guest_uctx->sc.ss;

    Frame->State.cs_cached = Frame->State.gdt[Frame->State.cs_idx >> 3].base;
    Frame->State.ds_cached = Frame->State.gdt[Frame->State.ds_idx >> 3].base;
    Frame->State.es_cached = Frame->State.gdt[Frame->State.es_idx >> 3].base;
    Frame->State.fs_cached = Frame->State.gdt[Frame->State.fs_idx >> 3].base;
    Frame->State.gs_cached = Frame->State.gdt[Frame->State.gs_idx >> 3].base;
    Frame->State.ss_cached = Frame->State.gdt[Frame->State.ss_idx >> 3].base;

#define COPY_REG(x, y) Frame->State.gregs[FEXCore::X86State::REG_##x] = guest_uctx->sc.y;
    COPY_REG(RDI, di);
    COPY_REG(RSI, si);
    COPY_REG(RBP, bp);
    COPY_REG(RBX, bx);
    COPY_REG(RDX, dx);
    COPY_REG(RAX, ax);
    COPY_REG(RCX, cx);
    COPY_REG(RSP, sp);
#undef COPY_REG
    auto* xstate = reinterpret_cast<FEXCore::x86::xstate*>(guest_uctx->sc.fpstate);
    auto* fpstate = &xstate->fpstate;

    // Copy float registers
    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_MMS; ++i) {
      // 32-bit st register size is only 10 bytes. Not padded to 16byte like x86-64
      memcpy(&Frame->State.mm[i], &fpstate->_st[i], 10);
    }

    // Extended XMM state
    if (IsAVXEnabled) {
      CTX->SetXMMRegistersFromState(Thread, fpstate->_xmm, xstate->ymmh.ymmh_space);
    } else {
      CTX->SetXMMRegistersFromState(Thread, fpstate->_xmm, nullptr);
    }

    // FCW store default
    Frame->State.FCW = fpstate->fcw;
    Frame->State.AbridgedFTW = FEXCore::FPState::ConvertToAbridgedFTW(fpstate->ftw);

    // Deconstruct FSW
    Frame->State.flags[FEXCore::X86State::X87FLAG_C0_LOC] = (fpstate->fsw >> 8) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_C1_LOC] = (fpstate->fsw >> 9) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_C2_LOC] = (fpstate->fsw >> 10) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_C3_LOC] = (fpstate->fsw >> 14) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] = (fpstate->fsw >> 11) & 0b111;
  }
}

void SignalDelegator::RestoreRTFrame_ia32(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* Context,
                                          FEXCore::Core::CpuStateFrame* Frame, void* ucontext) {
  const bool IsAVXEnabled = Config.SupportsAVX;

  RTSigFrame_i32* guest_uctx = reinterpret_cast<RTSigFrame_i32*>(Context->UContextLocation);
  // If the guest modified the RIP then we need to take special precautions here
  if (Context->OriginalRIP != guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_EIP] || Context->FaultToTopAndGeneratedException) {

    // Restore previous `InSyscallInfo` structure.
    Frame->InSyscallInfo = Context->InSyscallInfo;

    // Hack! Go back to the top of the dispatcher top
    // This is only safe inside the JIT rather than anything outside of it
    ArchHelpers::Context::SetPc(ucontext, Config.AbsoluteLoopTopAddressFillSRA);
    // Set our state register to point to our guest thread data
    ArchHelpers::Context::SetState(ucontext, reinterpret_cast<uint64_t>(Frame));

    // XXX: Full context setting
    CTX->SetFlagsFromCompactedEFLAGS(Thread, guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_EFL]);

    Frame->State.rip = guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_EIP];
    Frame->State.cs_idx = guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_CS];
    Frame->State.ds_idx = guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_DS];
    Frame->State.es_idx = guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_ES];
    Frame->State.fs_idx = guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_FS];
    Frame->State.gs_idx = guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_GS];
    Frame->State.ss_idx = guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_SS];

    Frame->State.cs_cached = Frame->State.gdt[Frame->State.cs_idx >> 3].base;
    Frame->State.ds_cached = Frame->State.gdt[Frame->State.ds_idx >> 3].base;
    Frame->State.es_cached = Frame->State.gdt[Frame->State.es_idx >> 3].base;
    Frame->State.fs_cached = Frame->State.gdt[Frame->State.fs_idx >> 3].base;
    Frame->State.gs_cached = Frame->State.gdt[Frame->State.gs_idx >> 3].base;
    Frame->State.ss_cached = Frame->State.gdt[Frame->State.ss_idx >> 3].base;

#define COPY_REG(x) Frame->State.gregs[FEXCore::X86State::REG_##x] = guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_##x];
    COPY_REG(RDI);
    COPY_REG(RSI);
    COPY_REG(RBP);
    COPY_REG(RBX);
    COPY_REG(RDX);
    COPY_REG(RAX);
    COPY_REG(RCX);
    COPY_REG(RSP);
#undef COPY_REG
    auto* xstate = reinterpret_cast<FEXCore::x86::xstate*>(guest_uctx->uc.uc_mcontext.fpregs);
    auto* fpstate = &xstate->fpstate;

    // Copy float registers
    for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_MMS; ++i) {
      // 32-bit st register size is only 10 bytes. Not padded to 16byte like x86-64
      memcpy(&Frame->State.mm[i], &fpstate->_st[i], 10);
    }

    // Extended XMM state
    if (IsAVXEnabled) {
      CTX->SetXMMRegistersFromState(Thread, fpstate->_xmm, xstate->ymmh.ymmh_space);
    } else {
      CTX->SetXMMRegistersFromState(Thread, fpstate->_xmm, nullptr);
    }

    // FCW store default
    Frame->State.FCW = fpstate->fcw;
    Frame->State.AbridgedFTW = FEXCore::FPState::ConvertToAbridgedFTW(fpstate->ftw);

    // Deconstruct FSW
    Frame->State.flags[FEXCore::X86State::X87FLAG_C0_LOC] = (fpstate->fsw >> 8) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_C1_LOC] = (fpstate->fsw >> 9) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_C2_LOC] = (fpstate->fsw >> 10) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_C3_LOC] = (fpstate->fsw >> 14) & 1;
    Frame->State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] = (fpstate->fsw >> 11) & 0b111;
  }
}

void SetupSigInfo_x64(siginfo_t* guest_siginfo, siginfo_t* HostSigInfo, bool WasFaultToTop,
                      FEXCore::Core::CpuStateFrame::SynchronousFaultDataStruct* SynchronousFaultData) {
  // aarch64 and x86_64 siginfo_t matches. We can just copy this over
  // SI_USER could also potentially have random data in it, needs to be bit perfect
  // For guest faults we don't have a real way to reconstruct state to a real guest RIP
  *guest_siginfo = *HostSigInfo;

  if (WasFaultToTop) {
    // Overwrite si_code
    guest_siginfo->si_code = SynchronousFaultData->si_code;
  }
}

struct ContextData {
  uint64_t OriginalRIP;
  FEXCore::Context::Context* CTX;
  FEXCore::Core::InternalThreadState* Thread;
  FEXCore::Core::CpuStateFrame* Frame;
  FEXCore::x86_64::xstate* fp_state;
  FEXCore::Core::CpuStateFrame::SynchronousFaultDataStruct* SynchronousFaultData;
  stack_t* GuestStack;
  void* HostUContext;
  siginfo_t* HostSigInfo;
  uint32_t eflags;
  int Signal;
  bool WasFaultToTop;
  bool IsAVXEnabled;
};

void SetupUContext_x64(FEXCore::x86_64::ucontext_t* guest_uctx, const ContextData& Data) {
  // We have extended float information
  guest_uctx->uc_flags = FEXCore::x86_64::UC_FP_XSTATE | FEXCore::x86_64::UC_SIGCONTEXT_SS | FEXCore::x86_64::UC_STRICT_RESTORE_SS;

  // Pointer to where the fpreg memory is
  guest_uctx->uc_mcontext.fpregs = reinterpret_cast<FEXCore::x86_64::_libc_fpstate*>(Data.fp_state);
  SetXStateInfo(Data.fp_state, Data.IsAVXEnabled);

  guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RIP] = Data.OriginalRIP;
  guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_EFL] = Data.eflags;
  guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_CSGSFS] = 0;

  if (Data.WasFaultToTop) {
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_TRAPNO] = Data.SynchronousFaultData->TrapNo;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_ERR] = Data.SynchronousFaultData->err_code;
  } else {
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_TRAPNO] = ConvertSignalToTrapNo(Data.Signal, Data.HostSigInfo);
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_ERR] = ConvertSignalToError(Data.HostUContext, Data.Signal, Data.HostSigInfo);
  }

  guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_OLDMASK] = 0;
  guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_CR2] = 0;

#define COPY_REG(x) guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_##x] = Data.Frame->State.gregs[FEXCore::X86State::REG_##x];
  COPY_REG(R8);
  COPY_REG(R9);
  COPY_REG(R10);
  COPY_REG(R11);
  COPY_REG(R12);
  COPY_REG(R13);
  COPY_REG(R14);
  COPY_REG(R15);
  COPY_REG(RDI);
  COPY_REG(RSI);
  COPY_REG(RBP);
  COPY_REG(RBX);
  COPY_REG(RDX);
  COPY_REG(RAX);
  COPY_REG(RCX);
  COPY_REG(RSP);
#undef COPY_REG

  auto* fpstate = &Data.fp_state->fpstate;

  // Copy float registers
  memcpy(fpstate->_st, Data.Frame->State.mm, sizeof(Data.Frame->State.mm));

  if (Data.IsAVXEnabled) {
    Data.CTX->ReconstructXMMRegisters(Data.Thread, fpstate->_xmm, Data.fp_state->ymmh.ymmh_space);
  } else {
    Data.CTX->ReconstructXMMRegisters(Data.Thread, fpstate->_xmm, nullptr);
  }

  // FCW store default
  fpstate->fcw = Data.Frame->State.FCW;
  fpstate->ftw = Data.Frame->State.AbridgedFTW;

  // Reconstruct FSW
  fpstate->fsw =
    (Data.Frame->State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] << 11) |
    (Data.Frame->State.flags[FEXCore::X86State::X87FLAG_C0_LOC] << 8) | (Data.Frame->State.flags[FEXCore::X86State::X87FLAG_C1_LOC] << 9) |
    (Data.Frame->State.flags[FEXCore::X86State::X87FLAG_C2_LOC] << 10) | (Data.Frame->State.flags[FEXCore::X86State::X87FLAG_C3_LOC] << 14);

  // Copy over signal stack information
  guest_uctx->uc_stack.ss_flags = Data.GuestStack->ss_flags;
  guest_uctx->uc_stack.ss_sp = Data.GuestStack->ss_sp;
  guest_uctx->uc_stack.ss_size = Data.GuestStack->ss_size;
}

uint64_t SignalDelegator::SetupFrame_x64(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* ContextBackup,
                                         FEXCore::Core::CpuStateFrame* Frame, int Signal, siginfo_t* HostSigInfo, void* ucontext,
                                         GuestSigAction* GuestAction, stack_t* GuestStack, uint64_t NewGuestSP, const uint32_t eflags) {

  // Back up past the redzone, which is 128bytes
  // 32-bit doesn't have a redzone
  NewGuestSP -= 128;

  const bool IsAVXEnabled = Config.SupportsAVX;

  // On 64-bit the kernel sets up the siginfo_t and ucontext_t regardless of SA_SIGINFO set.
  // This allows the application to /always/ get the siginfo and ucontext even if it didn't set this flag.
  //
  // Signal frame layout on stack needs to be as follows
  // void* ReturnPointer
  // ucontext_t
  // siginfo_t
  // FP state
  // Host stack location
  NewGuestSP -= sizeof(uint64_t);
  NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(uint64_t));

  uint64_t HostStackLocation = NewGuestSP;

  if (IsAVXEnabled) {
    NewGuestSP -= sizeof(FEXCore::x86_64::xstate);
    NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(FEXCore::x86_64::xstate));
  } else {
    NewGuestSP -= sizeof(FEXCore::x86_64::_libc_fpstate);
    NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(FEXCore::x86_64::_libc_fpstate));
  }

  uint64_t FPStateLocation = NewGuestSP;

  NewGuestSP -= sizeof(siginfo_t);
  NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(siginfo_t));
  uint64_t SigInfoLocation = NewGuestSP;

  NewGuestSP -= sizeof(FEXCore::x86_64::ucontext_t);
  NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(FEXCore::x86_64::ucontext_t));
  uint64_t UContextLocation = NewGuestSP;

  ContextBackup->FPStateLocation = FPStateLocation;
  ContextBackup->UContextLocation = UContextLocation;
  ContextBackup->SigInfoLocation = SigInfoLocation;

  // Store where the host context lives in the guest stack.
  *(uint64_t*)HostStackLocation = (uint64_t)ContextBackup;

  if (ContextBackup->FaultToTopAndGeneratedException) {
    Signal = Frame->SynchronousFaultData.Signal;
  }

  FEXCore::x86_64::ucontext_t* guest_uctx = reinterpret_cast<FEXCore::x86_64::ucontext_t*>(UContextLocation);
  siginfo_t* guest_siginfo = reinterpret_cast<siginfo_t*>(SigInfoLocation);
  auto* xstate = reinterpret_cast<FEXCore::x86_64::xstate*>(FPStateLocation);
  ContextData Data {.OriginalRIP = ContextBackup->OriginalRIP,
                    .CTX = CTX,
                    .Thread = Thread,
                    .Frame = Frame,
                    .fp_state = xstate,
                    .SynchronousFaultData = &Frame->SynchronousFaultData,
                    .GuestStack = GuestStack,
                    .HostUContext = ucontext,
                    .HostSigInfo = HostSigInfo,
                    .eflags = eflags,
                    .Signal = Signal,
                    .WasFaultToTop = ContextBackup->FaultToTopAndGeneratedException,
                    .IsAVXEnabled = IsAVXEnabled};
  SetupUContext_x64(guest_uctx, Data);

  SetupSigInfo_x64(guest_siginfo, HostSigInfo, Data.WasFaultToTop, &Frame->SynchronousFaultData);

  // Apparently RAX is always set to zero in case of badly misbehaving C applications and variadics.
  Frame->State.gregs[FEXCore::X86State::REG_RAX] = 0;
  Frame->State.gregs[FEXCore::X86State::REG_RDI] = Signal;
  Frame->State.gregs[FEXCore::X86State::REG_RSI] = SigInfoLocation;
  Frame->State.gregs[FEXCore::X86State::REG_RDX] = UContextLocation;

  // Set up the new SP for stack handling
  // The host is required to provide us a restorer.
  // If the guest didn't provide a restorer then the application should fail with a SIGSEGV.
  // TODO: Emulate SIGSEGV when the guest doesn't provide a restorer.
  NewGuestSP -= 8;
  if (GuestAction->restorer) {
    *(uint64_t*)NewGuestSP = (uint64_t)GuestAction->restorer;
  } else {
    // XXX: Emulate SIGSEGV here
    // *(uint64_t*)NewGuestSP = SignalReturn;
  }

  return NewGuestSP;
}

uint64_t SignalDelegator::SetupFrame_ia32(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* ContextBackup,
                                          FEXCore::Core::CpuStateFrame* Frame, int Signal, siginfo_t* HostSigInfo, void* ucontext,
                                          GuestSigAction* GuestAction, stack_t* GuestStack, uint64_t NewGuestSP, const uint32_t eflags) {

  const bool IsAVXEnabled = Config.SupportsAVX;
  const uint64_t SignalReturn = reinterpret_cast<uint64_t>(VDSOPointers.VDSO_kernel_sigreturn);

  NewGuestSP -= sizeof(uint64_t);
  NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(uint64_t));

  uint64_t HostStackLocation = NewGuestSP;

  if (IsAVXEnabled) {
    NewGuestSP -= sizeof(FEXCore::x86::xstate);
    NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(FEXCore::x86::xstate));
  } else {
    NewGuestSP -= sizeof(FEXCore::x86::_libc_fpstate);
    NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(FEXCore::x86::_libc_fpstate));
  }

  uint64_t FPStateLocation = NewGuestSP;

  NewGuestSP -= sizeof(SigFrame_i32);
  NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(SigFrame_i32));
  uint64_t SigFrameLocation = NewGuestSP;

  ContextBackup->FPStateLocation = FPStateLocation;
  ContextBackup->UContextLocation = SigFrameLocation;
  ContextBackup->SigInfoLocation = 0;

  SigFrame_i32* guest_uctx = reinterpret_cast<SigFrame_i32*>(SigFrameLocation);
  // Store where the host context lives in the guest stack.
  *(uint64_t*)HostStackLocation = (uint64_t)ContextBackup;

  // Pointer to where the fpreg memory is
  guest_uctx->sc.fpstate = static_cast<uint32_t>(FPStateLocation);
  auto* xstate = reinterpret_cast<FEXCore::x86::xstate*>(FPStateLocation);
  SetXStateInfo(xstate, IsAVXEnabled);

  guest_uctx->sc.cs = Frame->State.cs_idx;
  guest_uctx->sc.ds = Frame->State.ds_idx;
  guest_uctx->sc.es = Frame->State.es_idx;
  guest_uctx->sc.fs = Frame->State.fs_idx;
  guest_uctx->sc.gs = Frame->State.gs_idx;
  guest_uctx->sc.ss = Frame->State.ss_idx;

  if (ContextBackup->FaultToTopAndGeneratedException) {
    guest_uctx->sc.trapno = Frame->SynchronousFaultData.TrapNo;
    guest_uctx->sc.err = Frame->SynchronousFaultData.err_code;
    Signal = Frame->SynchronousFaultData.Signal;
  } else {
    guest_uctx->sc.trapno = ConvertSignalToTrapNo(Signal, HostSigInfo);
    guest_uctx->sc.err = ConvertSignalToError(ucontext, Signal, HostSigInfo);
  }

  guest_uctx->sc.ip = ContextBackup->OriginalRIP;
  guest_uctx->sc.flags = eflags;
  guest_uctx->sc.sp_at_signal = 0;

#define COPY_REG(x, y) guest_uctx->sc.x = Frame->State.gregs[FEXCore::X86State::REG_##y];
  COPY_REG(di, RDI);
  COPY_REG(si, RSI);
  COPY_REG(bp, RBP);
  COPY_REG(bx, RBX);
  COPY_REG(dx, RDX);
  COPY_REG(ax, RAX);
  COPY_REG(cx, RCX);
  COPY_REG(sp, RSP);
#undef COPY_REG

  auto* fpstate = &xstate->fpstate;

  // Copy float registers
  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_MMS; ++i) {
    // 32-bit st register size is only 10 bytes. Not padded to 16byte like x86-64
    memcpy(&fpstate->_st[i], &Frame->State.mm[i], 10);
  }

  // Extended XMM state
  fpstate->status = FEXCore::x86::fpstate_magic::MAGIC_XFPSTATE;

  if (IsAVXEnabled) {
    CTX->ReconstructXMMRegisters(Thread, fpstate->_xmm, xstate->ymmh.ymmh_space);
  } else {
    CTX->ReconstructXMMRegisters(Thread, fpstate->_xmm, nullptr);
  }

  // FCW store default
  fpstate->fcw = Frame->State.FCW;
  // Reconstruct FSW
  fpstate->fsw = (Frame->State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] << 11) |
                 (Frame->State.flags[FEXCore::X86State::X87FLAG_C0_LOC] << 8) | (Frame->State.flags[FEXCore::X86State::X87FLAG_C1_LOC] << 9) |
                 (Frame->State.flags[FEXCore::X86State::X87FLAG_C2_LOC] << 10) | (Frame->State.flags[FEXCore::X86State::X87FLAG_C3_LOC] << 14);
  fpstate->ftw = FEXCore::FPState::ConvertFromAbridgedFTW(fpstate->fsw, Frame->State.mm, Frame->State.AbridgedFTW);

  // Curiously non-rt signals don't support altstack. So that state doesn't exist here.

  // Copy over the signal information.
  guest_uctx->Signal = Signal;

  // Retcode needs to be bit-exact for debuggers
  constexpr static uint8_t retcode[] = {
    0x58,                   // pop eax
    0xb8,                   // mov
    0x77, 0x00, 0x00, 0x00, // 32-bit sigreturn
    0xcd, 0x80,             // int 0x80
  };

  memcpy(guest_uctx->retcode, &retcode, sizeof(retcode));

  // 32-bit Guest can provide its own restorer or we need to provide our own.
  // On a real host this restorer will live in VDSO.
  constexpr uint32_t SA_RESTORER = 0x04000000;
  const bool HasRestorer = (GuestAction->sa_flags & SA_RESTORER) == SA_RESTORER;
  if (HasRestorer) {
    guest_uctx->pretcode = (uint32_t)(uint64_t)GuestAction->restorer;
  } else {
    guest_uctx->pretcode = SignalReturn;
    LOGMAN_THROW_AA_FMT(SignalReturn < 0x1'0000'0000ULL, "This needs to be below 4GB");
  }

  // Support regparm=3
  Frame->State.gregs[FEXCore::X86State::REG_RAX] = Signal;
  Frame->State.gregs[FEXCore::X86State::REG_RDX] = 0;
  Frame->State.gregs[FEXCore::X86State::REG_RCX] = 0;

  return NewGuestSP;
}

uint64_t SignalDelegator::SetupRTFrame_ia32(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* ContextBackup,
                                            FEXCore::Core::CpuStateFrame* Frame, int Signal, siginfo_t* HostSigInfo, void* ucontext,
                                            GuestSigAction* GuestAction, stack_t* GuestStack, uint64_t NewGuestSP, const uint32_t eflags) {

  const bool IsAVXEnabled = Config.SupportsAVX;
  const uint64_t SignalReturn = reinterpret_cast<uint64_t>(VDSOPointers.VDSO_kernel_rt_sigreturn);

  NewGuestSP -= sizeof(uint64_t);
  NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(uint64_t));

  uint64_t HostStackLocation = NewGuestSP;

  if (IsAVXEnabled) {
    NewGuestSP -= sizeof(FEXCore::x86::xstate);
    NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(FEXCore::x86::xstate));
  } else {
    NewGuestSP -= sizeof(FEXCore::x86::_libc_fpstate);
    NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(FEXCore::x86::_libc_fpstate));
  }

  uint64_t FPStateLocation = NewGuestSP;

  NewGuestSP -= sizeof(RTSigFrame_i32);
  NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(RTSigFrame_i32));

  uint64_t SigFrameLocation = NewGuestSP;
  RTSigFrame_i32* guest_uctx = reinterpret_cast<RTSigFrame_i32*>(SigFrameLocation);
  // Store where the host context lives in the guest stack.
  *(uint64_t*)HostStackLocation = (uint64_t)ContextBackup;

  ContextBackup->FPStateLocation = FPStateLocation;
  ContextBackup->UContextLocation = SigFrameLocation;
  ContextBackup->SigInfoLocation = 0; // Part of frame.

  // We have extended float information
  guest_uctx->uc.uc_flags = FEXCore::x86::UC_FP_XSTATE;
  guest_uctx->uc.uc_link = 0;

  // Pointer to where the fpreg memory is
  guest_uctx->uc.uc_mcontext.fpregs = static_cast<uint32_t>(FPStateLocation);
  auto* xstate = reinterpret_cast<FEXCore::x86::xstate*>(FPStateLocation);
  SetXStateInfo(xstate, IsAVXEnabled);

  guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_CS] = Frame->State.cs_idx;
  guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_DS] = Frame->State.ds_idx;
  guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_ES] = Frame->State.es_idx;
  guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_FS] = Frame->State.fs_idx;
  guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_GS] = Frame->State.gs_idx;
  guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_SS] = Frame->State.ss_idx;

  if (ContextBackup->FaultToTopAndGeneratedException) {
    guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_TRAPNO] = Frame->SynchronousFaultData.TrapNo;
    guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_ERR] = Frame->SynchronousFaultData.err_code;
    Signal = Frame->SynchronousFaultData.Signal;
  } else {
    guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_TRAPNO] = ConvertSignalToTrapNo(Signal, HostSigInfo);
    guest_uctx->info.si_code = HostSigInfo->si_code;
    guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_ERR] = ConvertSignalToError(ucontext, Signal, HostSigInfo);
  }

  guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_EIP] = ContextBackup->OriginalRIP;
  guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_EFL] = eflags;
  guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_UESP] = Frame->State.gregs[FEXCore::X86State::REG_RSP];
  guest_uctx->uc.uc_mcontext.cr2 = 0;

#define COPY_REG(x) guest_uctx->uc.uc_mcontext.gregs[FEXCore::x86::FEX_REG_##x] = Frame->State.gregs[FEXCore::X86State::REG_##x];
  COPY_REG(RDI);
  COPY_REG(RSI);
  COPY_REG(RBP);
  COPY_REG(RBX);
  COPY_REG(RDX);
  COPY_REG(RAX);
  COPY_REG(RCX);
  COPY_REG(RSP);
#undef COPY_REG

  auto* fpstate = &xstate->fpstate;

  // Copy float registers
  for (size_t i = 0; i < FEXCore::Core::CPUState::NUM_MMS; ++i) {
    // 32-bit st register size is only 10 bytes. Not padded to 16byte like x86-64
    memcpy(&fpstate->_st[i], &Frame->State.mm[i], 10);
  }

  // Extended XMM state
  fpstate->status = FEXCore::x86::fpstate_magic::MAGIC_XFPSTATE;

  if (IsAVXEnabled) {
    CTX->ReconstructXMMRegisters(Thread, fpstate->_xmm, xstate->ymmh.ymmh_space);
  } else {
    CTX->ReconstructXMMRegisters(Thread, fpstate->_xmm, nullptr);
  }

  // FCW store default
  fpstate->fcw = Frame->State.FCW;
  // Reconstruct FSW
  fpstate->fsw = (Frame->State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] << 11) |
                 (Frame->State.flags[FEXCore::X86State::X87FLAG_C0_LOC] << 8) | (Frame->State.flags[FEXCore::X86State::X87FLAG_C1_LOC] << 9) |
                 (Frame->State.flags[FEXCore::X86State::X87FLAG_C2_LOC] << 10) | (Frame->State.flags[FEXCore::X86State::X87FLAG_C3_LOC] << 14);
  fpstate->ftw = FEXCore::FPState::ConvertFromAbridgedFTW(fpstate->fsw, Frame->State.mm, Frame->State.AbridgedFTW);

  // Copy over signal stack information
  guest_uctx->uc.uc_stack.ss_flags = GuestStack->ss_flags;
  guest_uctx->uc.uc_stack.ss_sp = static_cast<uint32_t>(reinterpret_cast<uint64_t>(GuestStack->ss_sp));
  guest_uctx->uc.uc_stack.ss_size = GuestStack->ss_size;

  // Setup siginfo
  if (ContextBackup->FaultToTopAndGeneratedException) {
    guest_uctx->info.si_code = Frame->SynchronousFaultData.si_code;
  } else {
    guest_uctx->info.si_code = HostSigInfo->si_code;
  }

  // These three elements are in every siginfo
  guest_uctx->info.si_signo = HostSigInfo->si_signo;
  guest_uctx->info.si_errno = HostSigInfo->si_errno;

  const SigInfoLayout Layout = CalculateSigInfoLayout(Signal, guest_uctx->info.si_code);

  switch (Layout) {
  case SigInfoLayout::LAYOUT_KILL:
    guest_uctx->info._sifields._kill.pid = HostSigInfo->si_pid;
    guest_uctx->info._sifields._kill.uid = HostSigInfo->si_uid;
    break;
  case SigInfoLayout::LAYOUT_TIMER:
    guest_uctx->info._sifields._timer.tid = HostSigInfo->si_timerid;
    guest_uctx->info._sifields._timer.overrun = HostSigInfo->si_overrun;
    guest_uctx->info._sifields._timer.sigval.sival_int = HostSigInfo->si_int;
    break;
  case SigInfoLayout::LAYOUT_POLL:
    guest_uctx->info._sifields._poll.band = HostSigInfo->si_band;
    guest_uctx->info._sifields._poll.fd = HostSigInfo->si_fd;
    break;
  case SigInfoLayout::LAYOUT_FAULT:
    // Macro expansion to get the si_addr
    // This is the address trying to be accessed, not the RIP
    guest_uctx->info._sifields._sigfault.addr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(HostSigInfo->si_addr));
    break;
  case SigInfoLayout::LAYOUT_FAULT_RIP:
    // Macro expansion to get the si_addr
    // Can't really give a real result here. Pull from the context for now
    guest_uctx->info._sifields._sigfault.addr = ContextBackup->OriginalRIP;
    break;
  case SigInfoLayout::LAYOUT_CHLD:
    guest_uctx->info._sifields._sigchld.pid = HostSigInfo->si_pid;
    guest_uctx->info._sifields._sigchld.uid = HostSigInfo->si_uid;
    guest_uctx->info._sifields._sigchld.status = HostSigInfo->si_status;
    guest_uctx->info._sifields._sigchld.utime = HostSigInfo->si_utime;
    guest_uctx->info._sifields._sigchld.stime = HostSigInfo->si_stime;
    break;
  case SigInfoLayout::LAYOUT_RT:
    guest_uctx->info._sifields._rt.pid = HostSigInfo->si_pid;
    guest_uctx->info._sifields._rt.uid = HostSigInfo->si_uid;
    guest_uctx->info._sifields._rt.sigval.sival_int = HostSigInfo->si_int;
    break;
  case SigInfoLayout::LAYOUT_SYS:
    guest_uctx->info._sifields._sigsys.call_addr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(HostSigInfo->si_call_addr));
    guest_uctx->info._sifields._sigsys.syscall = HostSigInfo->si_syscall;
    // We need to lie about the architecture here.
    // Otherwise we would expose incorrect information to the guest.
    constexpr uint32_t AUDIT_LE = 0x4000'0000U;
    constexpr uint32_t MACHINE_I386 = 3; // This matches the ELF definition.
    guest_uctx->info._sifields._sigsys.arch = AUDIT_LE | MACHINE_I386;
    break;
  }

  // Setup the guest stack context.
  guest_uctx->Signal = Signal;
  guest_uctx->pinfo = (uint32_t)(uint64_t)&guest_uctx->info;
  guest_uctx->puc = (uint32_t)(uint64_t)&guest_uctx->uc;

  // Retcode needs to be bit-exact for debuggers
  constexpr static uint8_t rt_retcode[] = {
    0xb8,                   // mov
    0xad, 0x00, 0x00, 0x00, // 32-bit rt_sigreturn
    0xcd, 0x80,             // int 0x80
    0x0,                    // Pad
  };

  memcpy(guest_uctx->retcode, &rt_retcode, sizeof(rt_retcode));

  // 32-bit Guest can provide its own restorer or we need to provide our own.
  // On a real host this restorer will live in VDSO.
  constexpr uint32_t SA_RESTORER = 0x04000000;
  const bool HasRestorer = (GuestAction->sa_flags & SA_RESTORER) == SA_RESTORER;
  if (HasRestorer) {
    guest_uctx->pretcode = (uint32_t)(uint64_t)GuestAction->restorer;
  } else {
    guest_uctx->pretcode = SignalReturn;
    LOGMAN_THROW_AA_FMT(SignalReturn < 0x1'0000'0000ULL, "This needs to be below 4GB");
  }

  // Support regparm=3
  Frame->State.gregs[FEXCore::X86State::REG_RAX] = Signal;
  Frame->State.gregs[FEXCore::X86State::REG_RDX] = guest_uctx->pinfo;
  Frame->State.gregs[FEXCore::X86State::REG_RCX] = guest_uctx->puc;

  return NewGuestSP;
}

bool SignalDelegator::HandleDispatcherGuestSignal(FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext,
                                                  GuestSigAction* GuestAction, stack_t* GuestStack) {
  auto ContextBackup = StoreThreadState(Thread, Signal, ucontext);

  auto Frame = Thread->CurrentFrame;

  // Ref count our faults
  // We use this to track if it is safe to clear cache
  ++Thread->CurrentFrame->SignalHandlerRefCounter;

  uint64_t OldPC = ArchHelpers::Context::GetPc(ucontext);
  const bool WasInJIT = CTX->IsAddressInCodeBuffer(Thread, OldPC);

  // Spill the SRA regardless of signal handler type
  // We are going to be returning to the top of the dispatcher which will fill again
  // Otherwise we might load garbage
  if (WasInJIT) {
    uint32_t IgnoreMask {};
#ifdef _M_ARM_64
    if (Frame->InSyscallInfo != 0) {
      // We are in a syscall, this means we are in a weird register state
      // We need to spill SRA but only some of it, since some values have already been spilled
      // Lower 16 bits tells us which registers are already spilled to the context
      // So we ignore spilling those ones
      IgnoreMask = Frame->InSyscallInfo & 0xFFFF;
    } else {
      // We must spill everything
      IgnoreMask = 0;
    }
#endif

    // We are in jit, SRA must be spilled
    SpillSRA(Thread, ucontext, IgnoreMask);

    ContextBackup->Flags |= ArchHelpers::Context::ContextFlags::CONTEXT_FLAG_INJIT;

    // We are leaving the syscall information behind. Make sure to store the previous state.
    ContextBackup->InSyscallInfo = Thread->CurrentFrame->InSyscallInfo;
    Thread->CurrentFrame->InSyscallInfo = 0;
  } else {
    if (!IsAddressInDispatcher(OldPC)) {
      // This is likely to cause issues but in some cases it isn't fatal
      // This can also happen if we have put a signal on hold, then we just reenabled the signal
      // So we are in the syscall handler
      // Only throw a log message in this case
      if constexpr (false) {
        // XXX: Messages in the signal handler can cause us to crash
        LogMan::Msg::EFmt("Signals in dispatcher have unsynchronized context");
      }
    }
  }

  uint64_t OldGuestSP = Frame->State.gregs[FEXCore::X86State::REG_RSP];
  uint64_t NewGuestSP = OldGuestSP;

  // altstack is only used if the signal handler was setup with SA_ONSTACK
  if (GuestAction->sa_flags & SA_ONSTACK) {
    // Additionally the altstack is only used if the enabled (SS_DISABLE flag is not set)
    if (!(GuestStack->ss_flags & SS_DISABLE)) {
      // If our guest is already inside of the alternative stack
      // Then that means we are hitting recursive signals and we need to walk back the stack correctly
      uint64_t AltStackBase = reinterpret_cast<uint64_t>(GuestStack->ss_sp);
      uint64_t AltStackEnd = AltStackBase + GuestStack->ss_size;
      if (OldGuestSP >= AltStackBase && OldGuestSP <= AltStackEnd) {
        // We are already in the alt stack, the rest of the code will handle adjusting this
      } else {
        NewGuestSP = AltStackEnd;
      }
    }
  }

  // siginfo_t
  siginfo_t* HostSigInfo = reinterpret_cast<siginfo_t*>(info);

  // Backup where we think the RIP currently is
  ContextBackup->OriginalRIP = CTX->RestoreRIPFromHostPC(Thread, ArchHelpers::Context::GetPc(ucontext));
  // Calculate eflags upfront.
  uint32_t eflags = CTX->ReconstructCompactedEFLAGS(Thread, WasInJIT, ArchHelpers::Context::GetArmGPRs(ucontext),
                                                    ArchHelpers::Context::GetArmPState(ucontext));

  if (Is64BitMode) {
    NewGuestSP = SetupFrame_x64(Thread, ContextBackup, Frame, Signal, HostSigInfo, ucontext, GuestAction, GuestStack, NewGuestSP, eflags);
  } else {
    const bool SigInfoFrame = (GuestAction->sa_flags & SA_SIGINFO) == SA_SIGINFO;
    if (SigInfoFrame) {
      NewGuestSP = SetupRTFrame_ia32(Thread, ContextBackup, Frame, Signal, HostSigInfo, ucontext, GuestAction, GuestStack, NewGuestSP, eflags);
    } else {
      NewGuestSP = SetupFrame_ia32(Thread, ContextBackup, Frame, Signal, HostSigInfo, ucontext, GuestAction, GuestStack, NewGuestSP, eflags);
    }
  }

  Frame->State.rip = reinterpret_cast<uint64_t>(GuestAction->sigaction_handler.sigaction);
  Frame->State.gregs[FEXCore::X86State::REG_RSP] = NewGuestSP;

  // The guest starts its signal frame with a zero initialized FPU
  // Set that up now. Little bit costly but it's a requirement
  // This state will be restored on rt_sigreturn
  memset(Frame->State.xmm.avx.data, 0, sizeof(Frame->State.xmm));
  memset(Frame->State.mm, 0, sizeof(Frame->State.mm));
  Frame->State.FCW = 0x37F;
  Frame->State.AbridgedFTW = 0;

  // Set the new PC
  ArchHelpers::Context::SetPc(ucontext, Config.AbsoluteLoopTopAddressFillSRA);
  // Set our state register to point to our guest thread data
  ArchHelpers::Context::SetState(ucontext, reinterpret_cast<uint64_t>(Frame));

  return true;
}

bool SignalDelegator::HandleSIGILL(FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) {
  if (ArchHelpers::Context::GetPc(ucontext) == Config.SignalHandlerReturnAddress ||
      ArchHelpers::Context::GetPc(ucontext) == Config.SignalHandlerReturnAddressRT) {
    RestoreThreadState(Thread, ucontext,
                       ArchHelpers::Context::GetPc(ucontext) == Config.SignalHandlerReturnAddressRT ? RestoreType::TYPE_REALTIME :
                                                                                                      RestoreType::TYPE_NONREALTIME);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --Thread->CurrentFrame->SignalHandlerRefCounter;

    if (Thread->DeferredSignalFrames.size() != 0) {
      // If we have more deferred frames to process then mprotect back to PROT_NONE.
      // It will have been RW coming in to this sigreturn and now we need to remove permissions
      // to ensure FEX trampolines back to the SIGSEGV deferred handler.
      mprotect(reinterpret_cast<void*>(&Thread->InterruptFaultPage), sizeof(Thread->InterruptFaultPage), PROT_NONE);
    }
    return true;
  }

  if (ArchHelpers::Context::GetPc(ucontext) == Config.PauseReturnInstruction) {
    RestoreThreadState(Thread, ucontext, RestoreType::TYPE_PAUSE);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --Thread->CurrentFrame->SignalHandlerRefCounter;
    return true;
  }

  return false;
}

bool SignalDelegator::HandleSignalPause(FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) {
  FEXCore::Core::SignalEvent SignalReason = Thread->SignalReason.load();
  auto Frame = Thread->CurrentFrame;

  if (SignalReason == FEXCore::Core::SignalEvent::Pause) {
    // Store our thread state so we can come back to this
    StoreThreadState(Thread, Signal, ucontext);

    if (CTX->IsAddressInCodeBuffer(Thread, ArchHelpers::Context::GetPc(ucontext))) {
      // We are in jit, SRA must be spilled
      ArchHelpers::Context::SetPc(ucontext, Config.ThreadPauseHandlerAddressSpillSRA);
    } else {
      // We are in non-jit, SRA is already spilled
      LOGMAN_THROW_A_FMT(!IsAddressInDispatcher(ArchHelpers::Context::GetPc(ucontext)), "Signals in dispatcher have unsynchronized "
                                                                                        "context");
      ArchHelpers::Context::SetPc(ucontext, Config.ThreadPauseHandlerAddress);
    }

    // Set our state register to point to our guest thread data
    ArchHelpers::Context::SetState(ucontext, reinterpret_cast<uint64_t>(Frame));

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    ++Thread->CurrentFrame->SignalHandlerRefCounter;

    Thread->SignalReason.store(FEXCore::Core::SignalEvent::Nothing);
    return true;
  }

  if (SignalReason == FEXCore::Core::SignalEvent::Stop) {
    // Our thread is stopping
    // We don't care about anything at this point
    // Set the stack to our starting location when we entered the core and get out safely
    ArchHelpers::Context::SetSp(ucontext, Frame->ReturningStackLocation);

    // Our ref counting doesn't matter anymore
    Thread->CurrentFrame->SignalHandlerRefCounter = 0;

    // Set the new PC
    if (CTX->IsAddressInCodeBuffer(Thread, ArchHelpers::Context::GetPc(ucontext))) {
      // We are in jit, SRA must be spilled
      ArchHelpers::Context::SetPc(ucontext, Config.ThreadStopHandlerAddressSpillSRA);
    } else {
      // We are in non-jit, SRA is already spilled
      LOGMAN_THROW_A_FMT(!IsAddressInDispatcher(ArchHelpers::Context::GetPc(ucontext)), "Signals in dispatcher have unsynchronized "
                                                                                        "context");
      ArchHelpers::Context::SetPc(ucontext, Config.ThreadStopHandlerAddress);
    }

    // We need to be a little bit careful here
    // If we were already paused (due to GDB) and we are immediately stopping (due to gdb kill)
    // Then we need to ensure we don't double decrement our idle thread counter
    if (Thread->RunningEvents.ThreadSleeping) {
      // If the thread was sleeping then its idle counter was decremented
      // Reincrement it here to not break logic
      FEX::HLE::_SyscallHandler->TM.IncrementIdleRefCount();
    }

    Thread->SignalReason.store(FEXCore::Core::SignalEvent::Nothing);
    return true;
  }

  if (SignalReason == FEXCore::Core::SignalEvent::Return || SignalReason == FEXCore::Core::SignalEvent::ReturnRT) {
    RestoreThreadState(Thread, ucontext,
                       SignalReason == FEXCore::Core::SignalEvent::ReturnRT ? RestoreType::TYPE_REALTIME : RestoreType::TYPE_NONREALTIME);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --Thread->CurrentFrame->SignalHandlerRefCounter;

    Thread->SignalReason.store(FEXCore::Core::SignalEvent::Nothing);
    return true;
  }
  return false;
}

void SignalDelegator::SignalThread(FEXCore::Core::InternalThreadState* Thread, FEXCore::Core::SignalEvent Event) {
  auto ThreadObject = static_cast<const FEX::HLE::ThreadStateObject*>(Thread->FrontendPtr);
  if (Event == FEXCore::Core::SignalEvent::Pause && Thread->RunningEvents.Running.load() == false) {
    // Skip signaling a thread if it is already paused.
    return;
  }
  Thread->SignalReason.store(Event);
  FHU::Syscalls::tgkill(ThreadObject->ThreadInfo.PID, ThreadObject->ThreadInfo.TID, SignalDelegator::SIGNAL_FOR_PAUSE);
}

/**  @} */

static bool IsAsyncSignal(const siginfo_t* Info, int Signal) {
  if (Info->si_code <= SI_USER) {
    // If the signal is not from the kernel then it is always async.
    // This is because synchronous signals can be sent through tgkill,sigqueue and other methods.
    // SI_USER == 0 and all negative si_code values come from the user.
    return true;
  } else {
    // If the signal is from the kernel then it is async only if it isn't an explicit synchronous signal.
    switch (Signal) {
    // These are all synchronous signals.
    case SIGBUS:
    case SIGFPE:
    case SIGILL:
    case SIGSEGV:
    case SIGTRAP: return false;
    default: break;
    }
  }

  // Everything else is async and can be deferred.
  return true;
}

void SignalDelegator::HandleGuestSignal(FEXCore::Core::InternalThreadState* Thread, int Signal, void* Info, void* UContext) {
  ucontext_t* _context = (ucontext_t*)UContext;
  auto SigInfo = *static_cast<siginfo_t*>(Info);

  constexpr bool SupportDeferredSignals = true;
  if (SupportDeferredSignals) {
    auto MustDeferSignal = (Thread->CurrentFrame->State.DeferredSignalRefCount.Load() != 0);

    if (Signal == SIGSEGV && SigInfo.si_code == SEGV_ACCERR && SigInfo.si_addr == reinterpret_cast<void*>(&Thread->InterruptFaultPage)) {
      if (!MustDeferSignal) {
        // We just reached the end of the outermost signal-deferring section and faulted to check for pending signals.
        // Pull a signal frame off the stack.

        mprotect(reinterpret_cast<void*>(&Thread->InterruptFaultPage), sizeof(Thread->InterruptFaultPage), PROT_READ | PROT_WRITE);

        if (Thread->DeferredSignalFrames.empty()) {
          // No signals to defer. Just set the fault page back to RW and continue execution.
          // This occurs as a minor race condition between the refcount decrement and the access to the fault page.
          return;
        }

        auto Top = Thread->DeferredSignalFrames.back();
        Signal = Top.Signal;
        SigInfo = Top.Info;
        Thread->DeferredSignalFrames.pop_back();

        // Until we re-protect the page to PROT_NONE, FEX will now *permanently* defer signals and /not/ check them.
        //
        // In order to return /back/ to a sane state, we wait for the rt_sigreturn to happen.
        // rt_sigreturn will check if there are any more deferred signals to handle
        // - If there are deferred signals
        //   - mprotect back to PROT_NONE
        //   - sigreturn will trampoline out to the previous fault address check, SIGSEGV and restart
        // - If there are *no* deferred signals
        //  - No need to mprotect, it is already RW
      } else {
#ifdef _M_ARM_64
        // If RefCount != 0 then that means we hit an access with nested signal-deferring sections.
        // Increment the PC past the `str zr, [x1]` to continue code execution until we reach the outermost section.
        ArchHelpers::Context::SetPc(UContext, ArchHelpers::Context::GetPc(UContext) + 4);
        return;
#else
        // X86 should always be doing a refcount compare and branch since we can't guarantee instruction size.
        // ARM64 just always does the access to reduce branching overhead.
        ERROR_AND_DIE_FMT("X86 shouldn't hit this InterruptFaultPage");
#endif
      }
    } else if (Signal == SIGSEGV && SigInfo.si_code == SEGV_ACCERR && FaultSafeMemcpy::IsFaultLocation(ArchHelpers::Context::GetPc(UContext))) {
      // If you want to emulate EFAULT behaviour then enable this if-statement.
      // Do this once we find an application that depends on this.
      if constexpr (false) {
        // Return from the subroutine, returning EFAULT.
        ArchHelpers::Context::SetArmReg(UContext, 0, EFAULT);
        ArchHelpers::Context::SetPc(UContext, ArchHelpers::Context::GetArmReg(UContext, 30));
        return;
      } else {
        LogMan::Msg::AFmt("Received invalid data to syscall. Crashing now!");
      }
    } else {
      if (IsAsyncSignal(&SigInfo, Signal) && MustDeferSignal) {
        // If the signal is asynchronous (as determined by si_code) and FEX is in a state of needing
        // to defer the signal, then add the signal to the thread's signal queue.
        LOGMAN_THROW_A_FMT(Thread->DeferredSignalFrames.size() != Thread->DeferredSignalFrames.capacity(), "Deferred signals vector hit "
                                                                                                           "capacity size. This will "
                                                                                                           "likely crash! Asserting now!");
        Thread->DeferredSignalFrames.emplace_back(FEXCore::Core::InternalThreadState::DeferredSignalState {
          .Info = SigInfo,
          .Signal = Signal,
        });

        // Now update the faulting page permissions so it will fault on write.
        mprotect(reinterpret_cast<void*>(&Thread->InterruptFaultPage), sizeof(Thread->InterruptFaultPage), PROT_NONE);

        // Postpone the remainder of signal handling logic until we process the SIGSEGV triggered by writing to InterruptFaultPage.
        return;
      }
    }
  }
  // Let the host take first stab at handling the signal
  SignalHandler& Handler = HostHandlers[Signal];

  // Remove the pending signal
  ThreadData.PendingSignals &= ~(1ULL << (Signal - 1));

  // We have an emulation thread pointer, we can now modify its state
  if (Handler.GuestAction.sigaction_handler.handler == SIG_DFL) {
    if (Handler.DefaultBehaviour == DEFAULT_TERM || Handler.DefaultBehaviour == DEFAULT_COREDUMP) {
      // Let the signal fall through to the unhandled path
      // This way the parent process can know it died correctly
    }
  } else if (Handler.GuestAction.sigaction_handler.handler == SIG_IGN) {
    return;
  } else {
    if (Handler.GuestHandler && Handler.GuestHandler(Thread, Signal, &SigInfo, UContext, &Handler.GuestAction, &ThreadData.GuestAltStack)) {
      // Set up a new mask based on this signals signal mask
      uint64_t NewMask = Handler.GuestAction.sa_mask.Val;

      // If NODEFER then the new signal mask includes this signal
      if (!(Handler.GuestAction.sa_flags & SA_NODEFER)) {
        NewMask |= (1ULL << (Signal - 1));
      }

      // Walk our required signals and stop masking them if requested
      for (size_t i = 0; i < MAX_SIGNALS; ++i) {
        if (HostHandlers[i + 1].Required.load(std::memory_order_relaxed)) {
          // Never mask our required signals
          NewMask &= ~(1ULL << i);
        }
      }

      // Update our host signal mask so we don't hit race conditions with signals
      // This allows us to maintain the expected signal mask through the guest signal handling and then all the way back again
      memcpy(&_context->uc_sigmask, &NewMask, sizeof(uint64_t));

      // We handled this signal, continue running
      return;
    }
    ERROR_AND_DIE_FMT("Unhandled guest exception");
  }

  // Unhandled crash
  // Call back in to the previous handler
  if (Handler.OldAction.sa_flags & SA_SIGINFO) {
    Handler.OldAction.sigaction(Signal, &SigInfo, UContext);
  } else if (Handler.OldAction.handler == SIG_IGN || (Handler.OldAction.handler == SIG_DFL && Handler.DefaultBehaviour == DEFAULT_IGNORE)) {
    // Do nothing
  } else if (Handler.OldAction.handler == SIG_DFL && (Handler.DefaultBehaviour == DEFAULT_COREDUMP || Handler.DefaultBehaviour == DEFAULT_TERM)) {

#ifndef FEX_DISABLE_TELEMETRY
    // In the case of signals that cause coredump or terminate, save telemetry early.
    // FEX is hard crashing at this point and won't hit regular shutdown routines.
    // Add the signal to the crash mask.
    CrashMask |= (1ULL << Signal);
    if (Signal == SIGSEGV && reinterpret_cast<uint64_t>(SigInfo.si_addr) >= SyscallHandler::TASK_MAX_64BIT) {
      // Tried accessing invalid non-canonical x86-64 address.
      UnhandledNonCanonical = true;
    }
    SaveTelemetry();
#endif

    // Reassign back to DFL and crash
    signal(Signal, SIG_DFL);
    if (SigInfo.si_code != SI_KERNEL) {
      // If the signal wasn't sent by the kernel then we need to reraise it.
      // This is necessary since returning from this signal handler now might just continue executing.
      // eg: If sent from tgkill then the signal gets dropped and returns.
      FHU::Syscalls::tgkill(::getpid(), FHU::Syscalls::gettid(), Signal);
    }
  } else {
    Handler.OldAction.handler(Signal);
  }
}

void SignalDelegator::SaveTelemetry() {
#ifndef FEX_DISABLE_TELEMETRY
  if (!ApplicationName.empty()) {
    FEXCore::Telemetry::Shutdown(ApplicationName);
  }
#endif
}

bool SignalDelegator::InstallHostThunk(int Signal) {
  SignalHandler& SignalHandler = HostHandlers[Signal];
  // If the host thunk is already installed for this, just return
  if (SignalHandler.Installed) {
    return false;
  }

  // Default flags for us
  SignalHandler.HostAction.sa_flags = SA_SIGINFO | SA_ONSTACK;

  bool Result = UpdateHostThunk(Signal);

  SignalHandler.Installed = Result;
  return Result;
}

bool SignalDelegator::UpdateHostThunk(int Signal) {
  SignalHandler& SignalHandler = HostHandlers[Signal];

  // Now install the thunk handler
  SignalHandler.HostAction.sigaction = SignalHandlerThunk;

  auto CheckAndAddFlags = [](uint64_t HostFlags, uint64_t GuestFlags, uint64_t Flags) {
    // If any of the flags don't match then update to the newest set
    if ((HostFlags ^ GuestFlags) & Flags) {
      // Remove all the flags from the host that we are testing for
      HostFlags &= ~Flags;
      // Copy over the guest flags being set
      HostFlags |= GuestFlags & Flags;
    }

    return HostFlags;
  };

  // Don't allow the guest to override flags for
  // SA_SIGINFO : Host always needs SA_SIGINFO
  // SA_ONSTACK : Host always needs the altstack
  // SA_RESETHAND : We don't support one shot handlers
  // SA_RESTORER : We always need our host side restorer on x86-64, Couldn't use guest restorer anyway
  SignalHandler.HostAction.sa_flags = CheckAndAddFlags(SignalHandler.HostAction.sa_flags, SignalHandler.GuestAction.sa_flags,
                                                       SA_NOCLDSTOP | SA_NOCLDWAIT | SA_NODEFER | SA_RESTART);

#ifdef _M_X86_64
#define SA_RESTORER 0x04000000
  SignalHandler.HostAction.sa_flags |= SA_RESTORER;
  SignalHandler.HostAction.restorer = sigrestore;
#endif

  // Walk the signals we have that are required and make sure to remove it from the mask
  // This'll likely be SIGILL, SIGBUS, SIG63

  // If the guest has masked some signals then we need to also mask those signals
  for (size_t i = 1; i < HostHandlers.size(); ++i) {
    if (HostHandlers[i].Required.load(std::memory_order_relaxed)) {
      SignalHandler.HostAction.sa_mask &= ~(1ULL << (i - 1));
    } else if (SigIsMember(&SignalHandler.GuestAction.sa_mask, i)) {
      SignalHandler.HostAction.sa_mask |= (1ULL << (i - 1));
    }
  }

  // Check for SIG_IGN
  if (SignalHandler.GuestAction.sigaction_handler.handler == SIG_IGN && HostHandlers[Signal].Required.load(std::memory_order_relaxed) == false) {
    // We are ignoring this signal on the guest
    // Which means we need to ignore it on the host as well
    SignalHandler.HostAction.handler = SIG_IGN;
  }

  // Check for SIG_DFL
  if (SignalHandler.GuestAction.sigaction_handler.handler == SIG_DFL && HostHandlers[Signal].Required.load(std::memory_order_relaxed) == false) {
    // Default handler on guest and default handler on host
    // With coredump and terminate then expect fireworks, but that is what the guest wants
    SignalHandler.HostAction.handler = SIG_DFL;
  }

  // Only update the old action if we haven't ever been installed
  const int Result =
    ::syscall(SYS_rt_sigaction, Signal, &SignalHandler.HostAction, SignalHandler.Installed ? nullptr : &SignalHandler.OldAction, 8);
  if (Result < 0) {
    // Signal 32 and 33 are consumed by glibc. We don't handle this atm
    LogMan::Msg::AFmt("Failed to install host signal thunk for signal {}: {}", Signal, strerror(errno));
    return false;
  }

  return true;
}

void SignalDelegator::UninstallHostHandler(int Signal) {
  SignalHandler& SignalHandler = HostHandlers[Signal];

  ::syscall(SYS_rt_sigaction, Signal, &SignalHandler.OldAction, nullptr, 8);
}

SignalDelegator::SignalDelegator(FEXCore::Context::Context* _CTX, const std::string_view ApplicationName)
  : CTX {_CTX}
  , ApplicationName {ApplicationName} {
  // Register this delegate
  LOGMAN_THROW_AA_FMT(!GlobalDelegator, "Can't register global delegator multiple times!");
  GlobalDelegator = this;
  // Signal zero isn't real
  HostHandlers[0].Installed = true;

  // We can't capture SIGKILL or SIGSTOP
  HostHandlers[SIGKILL].Installed = true;
  HostHandlers[SIGSTOP].Installed = true;

  if (ParanoidTSO()) {
    UnalignedHandlerType = FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::Paranoid;
  } else if (HalfBarrierTSOEnabled()) {
    UnalignedHandlerType = FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::HalfBarrier;
  } else {
    UnalignedHandlerType = FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::NonAtomic;
  }

  // Most signals default to termination
  // These ones are slightly different
  static constexpr std::array<std::pair<int, SignalDelegator::DefaultBehaviour>, 14> SignalDefaultBehaviours = {{
    {SIGQUIT, DEFAULT_COREDUMP},
    {SIGILL, DEFAULT_COREDUMP},
    {SIGTRAP, DEFAULT_COREDUMP},
    {SIGABRT, DEFAULT_COREDUMP},
    {SIGBUS, DEFAULT_COREDUMP},
    {SIGFPE, DEFAULT_COREDUMP},
    {SIGSEGV, DEFAULT_COREDUMP},
    {SIGCHLD, DEFAULT_IGNORE},
    {SIGCONT, DEFAULT_IGNORE},
    {SIGURG, DEFAULT_IGNORE},
    {SIGXCPU, DEFAULT_COREDUMP},
    {SIGXFSZ, DEFAULT_COREDUMP},
    {SIGSYS, DEFAULT_COREDUMP},
    {SIGWINCH, DEFAULT_IGNORE},
  }};

  for (const auto& [Signal, Behaviour] : SignalDefaultBehaviours) {
    HostHandlers[Signal].DefaultBehaviour = Behaviour;
  }

  // Register frontend SIGILL handler for forced assertion.
  RegisterFrontendHostSignalHandler(
    SIGILL,
    [](FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) -> bool {
      ucontext_t* _context = (ucontext_t*)ucontext;
      auto& mcontext = _context->uc_mcontext;
      uint64_t PC {};
#ifdef _M_ARM_64
      PC = mcontext.pc;
#else
      PC = mcontext.gregs[REG_RIP];
#endif
      if (PC == reinterpret_cast<uint64_t>(&FEXCore::Assert::ForcedAssert)) {
        // This is a host side assert. Don't deliver this to the guest
        // We want to actually break here
        GlobalDelegator->UninstallHostHandler(Signal);
        return true;
      }
      return false;
    },
    true);

  const auto PauseHandler = [](FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) -> bool {
    return GlobalDelegator->HandleSignalPause(Thread, Signal, info, ucontext);
  };

  const auto GuestSignalHandler = [](FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext,
                                     GuestSigAction* GuestAction, stack_t* GuestStack) -> bool {
    return GlobalDelegator->HandleDispatcherGuestSignal(Thread, Signal, info, ucontext, GuestAction, GuestStack);
  };

  const auto SigillHandler = [](FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) -> bool {
    return GlobalDelegator->HandleSIGILL(Thread, Signal, info, ucontext);
  };

  // Register SIGILL signal handler.
  RegisterHostSignalHandler(SIGILL, SigillHandler, true);

#ifdef _M_ARM_64
  // Register SIGBUS signal handler.
  const auto SigbusHandler = [](FEXCore::Core::InternalThreadState* Thread, int Signal, void* _info, void* ucontext) -> bool {
    const auto PC = ArchHelpers::Context::GetPc(ucontext);
    if (!Thread->CTX->IsAddressInCodeBuffer(Thread, PC)) {
      // Wasn't a sigbus in JIT code
      return false;
    }
    siginfo_t* info = reinterpret_cast<siginfo_t*>(_info);

    if (info->si_code != BUS_ADRALN) {
      // This only handles alignment problems
      return false;
    }

    const auto Result = FEXCore::ArchHelpers::Arm64::HandleUnalignedAccess(Thread, GlobalDelegator->GetUnalignedHandlerType(), PC,
                                                                           ArchHelpers::Context::GetArmGPRs(ucontext));
    ArchHelpers::Context::SetPc(ucontext, PC + Result.second);
    return Result.first;
  };

  RegisterHostSignalHandler(SIGBUS, SigbusHandler, true);
#endif
  // Register pause signal handler.
  RegisterHostSignalHandler(SignalDelegator::SIGNAL_FOR_PAUSE, PauseHandler, true);

  // Guest signal handlers.
  for (uint32_t Signal = 0; Signal <= SignalDelegator::MAX_SIGNALS; ++Signal) {
    RegisterHostSignalHandlerForGuest(Signal, GuestSignalHandler);
  }
}

SignalDelegator::~SignalDelegator() {
  for (int i = 0; i < MAX_SIGNALS; ++i) {
    if (i == 0 || i == SIGKILL || i == SIGSTOP || !HostHandlers[i].Installed) {
      continue;
    }
    ::syscall(SYS_rt_sigaction, i, &HostHandlers[i].OldAction, nullptr, 8);
    HostHandlers[i].Installed = false;
  }
  GlobalDelegator = nullptr;
}

FEX::HLE::ThreadStateObject* SignalDelegator::GetTLSThread() {
  return ThreadData.Thread;
}

void SignalDelegator::RegisterTLSState(FEX::HLE::ThreadStateObject* Thread) {
  ThreadData.Thread = Thread;

  // Set up our signal alternative stack
  // This is per thread rather than per signal
  ThreadData.AltStackPtr = FEXCore::Allocator::mmap(nullptr, SIGSTKSZ * 16, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  stack_t altstack {};
  altstack.ss_sp = ThreadData.AltStackPtr;
  altstack.ss_size = SIGSTKSZ * 16;
  altstack.ss_flags = 0;
  LOGMAN_THROW_AA_FMT(!!altstack.ss_sp, "Couldn't allocate stack pointer");

  // Register the alt stack
  const int Result = sigaltstack(&altstack, nullptr);
  if (Result == -1) {
    LogMan::Msg::EFmt("Failed to install alternative signal stack {}", strerror(errno));
  }

  // Get the current host signal mask
  ::syscall(SYS_rt_sigprocmask, 0, nullptr, &ThreadData.CurrentSignalMask.Val, 8);

  if (Thread->Thread) {
    // Reserve a small amount of deferred signal frames. Usually the stack won't be utilized beyond
    // 1 or 2 signals but add a few more just in case.
    Thread->Thread->DeferredSignalFrames.reserve(8);
  }
}

void SignalDelegator::UninstallTLSState(FEX::HLE::ThreadStateObject* Thread) {
  FEXCore::Allocator::munmap(ThreadData.AltStackPtr, SIGSTKSZ * 16);

  ThreadData.AltStackPtr = nullptr;

  stack_t altstack {};
  altstack.ss_flags = SS_DISABLE;

  // Uninstall the alt stack
  const int Result = sigaltstack(&altstack, nullptr);
  if (Result == -1) {
    LogMan::Msg::EFmt("Failed to uninstall alternative signal stack {}", strerror(errno));
  }

  ThreadData.Thread = nullptr;
}

void SignalDelegator::FrontendRegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
  // Linux signal handlers are per-process rather than per thread
  // Multiple threads could be calling in to this
  std::lock_guard lk(HostDelegatorMutex);
  HostHandlers[Signal].Required = Required;
  InstallHostThunk(Signal);
}

void SignalDelegator::FrontendRegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
  // Linux signal handlers are per-process rather than per thread
  // Multiple threads could be calling in to this
  std::lock_guard lk(HostDelegatorMutex);
  HostHandlers[Signal].Required = Required;
  InstallHostThunk(Signal);
}

void SignalDelegator::RegisterHostSignalHandlerForGuest(int Signal, FEX::HLE::HostSignalDelegatorFunctionForGuest Func) {
  std::lock_guard lk(HostDelegatorMutex);
  HostHandlers[Signal].GuestHandler = std::move(Func);
}

void SignalDelegator::RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
  SetFrontendHostSignalHandler(Signal, Func, Required);
  FrontendRegisterFrontendHostSignalHandler(Signal, Func, Required);
}

uint64_t SignalDelegator::RegisterGuestSignalHandler(int Signal, const GuestSigAction* Action, GuestSigAction* OldAction) {
  std::lock_guard lk(GuestDelegatorMutex);

  // Invalid signal specified
  if (Signal > MAX_SIGNALS) {
    return -EINVAL;
  }

  // If we have an old signal set then give it back
  if (OldAction) {
    *OldAction = HostHandlers[Signal].GuestAction;
  }

  // Now assign the new action
  if (Action) {
    // These signal dispositions can't be changed on Linux
    if (Signal == SIGKILL || Signal == SIGSTOP) {
      return -EINVAL;
    }

    HostHandlers[Signal].GuestAction = *Action;
    // Only attempt to install a new thunk handler if we were installing a new guest action
    if (!InstallHostThunk(Signal)) {
      UpdateHostThunk(Signal);
    }
  }

  return 0;
}

void SignalDelegator::CheckXIDHandler() {
  std::lock_guard lk(GuestDelegatorMutex);
  std::lock_guard lk2(HostDelegatorMutex);

  constexpr size_t SIGNAL_SETXID = 33;

  kernel_sigaction CurrentAction {};

  // Only update the old action if we haven't ever been installed
  const int Result = ::syscall(SYS_rt_sigaction, SIGNAL_SETXID, nullptr, &CurrentAction, 8);
  if (Result < 0) {
    LogMan::Msg::AFmt("Failed to get status of XID signal");
    return;
  }

  SignalHandler& HostHandler = HostHandlers[SIGNAL_SETXID];
  if (CurrentAction.handler != HostHandler.HostAction.handler) {
    // GLIBC overwrote our XID handler, reinstate our handler
    const int Result = ::syscall(SYS_rt_sigaction, SIGNAL_SETXID, &HostHandler.HostAction, nullptr, 8);
    if (Result < 0) {
      LogMan::Msg::AFmt("Failed to reinstate our XID signal handler {}", strerror(errno));
    }
  }
}

uint64_t SignalDelegator::RegisterGuestSigAltStack(const stack_t* ss, stack_t* old_ss) {
  auto Thread = GetTLSThread();
  bool UsingAltStack {};
  uint64_t AltStackBase = reinterpret_cast<uint64_t>(ThreadData.GuestAltStack.ss_sp);
  uint64_t AltStackEnd = AltStackBase + ThreadData.GuestAltStack.ss_size;
  uint64_t GuestSP = Thread->Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RSP];

  if (!(ThreadData.GuestAltStack.ss_flags & SS_DISABLE) && GuestSP >= AltStackBase && GuestSP <= AltStackEnd) {
    UsingAltStack = true;
  }

  // If we have an old signal set then give it back
  if (old_ss) {
    *old_ss = ThreadData.GuestAltStack;

    if (UsingAltStack) {
      // We are currently operating on the alt stack
      // Let the guest know
      old_ss->ss_flags |= SS_ONSTACK;
    } else {
      old_ss->ss_flags |= SS_DISABLE;
    }
  }

  // Now assign the new action
  if (ss) {
    // If we tried setting the alt stack while we are using it then throw an error
    if (UsingAltStack) {
      return -EPERM;
    }

    // We need to check for invalid flags
    // The only flag that can be passed is SS_AUTODISARM and SS_DISABLE
    if ((ss->ss_flags & ~SS_ONSTACK) & // SS_ONSTACK is ignored
        ~(SS_AUTODISARM | SS_DISABLE)) {
      // A flag remained that isn't one of the supported ones?
      return -EINVAL;
    }

    if (ss->ss_flags & SS_DISABLE) {
      // If SS_DISABLE Is specified then the rest of the details are ignored
      ThreadData.GuestAltStack = *ss;
      return 0;
    }

    // stack size needs to be MINSIGSTKSZ (0x2000)
    if (ss->ss_size < X86_MINSIGSTKSZ) {
      return -ENOMEM;
    }

    ThreadData.GuestAltStack = *ss;
  }

  return 0;
}

static void CheckForPendingSignals(FEXCore::Core::InternalThreadState* Thread) {
  auto ThreadObject = static_cast<const FEX::HLE::ThreadStateObject*>(Thread->FrontendPtr);

  // Do we have any pending signals that became unmasked?
  uint64_t PendingSignals = ~ThreadData.CurrentSignalMask.Val & ThreadData.PendingSignals;
  if (PendingSignals != 0) {
    for (int i = 0; i < 64; ++i) {
      if (PendingSignals & (1ULL << i)) {
        FHU::Syscalls::tgkill(ThreadObject->ThreadInfo.PID, ThreadObject->ThreadInfo.TID, i + 1);
        // We might not even return here which is spooky
      }
    }
  }
}

uint64_t SignalDelegator::GuestSigProcMask(int how, const uint64_t* set, uint64_t* oldset) {
  // The order in which we handle signal mask setting is important here
  // old and new can point to the same location in memory.
  // Even if the pointers are to same memory location, we must store the original signal mask
  // coming in to the syscall.
  // 1) Store old mask
  // 2) Set mask to new mask if exists
  // 3) Give old mask back
  auto OldSet = ThreadData.CurrentSignalMask.Val;

  if (!!set) {
    uint64_t IgnoredSignalsMask = ~((1ULL << (SIGKILL - 1)) | (1ULL << (SIGSTOP - 1)));
    if (how == SIG_BLOCK) {
      ThreadData.CurrentSignalMask.Val |= *set & IgnoredSignalsMask;
    } else if (how == SIG_UNBLOCK) {
      ThreadData.CurrentSignalMask.Val &= ~(*set & IgnoredSignalsMask);
    } else if (how == SIG_SETMASK) {
      ThreadData.CurrentSignalMask.Val = *set & IgnoredSignalsMask;
    } else {
      return -EINVAL;
    }

    uint64_t HostMask = ThreadData.CurrentSignalMask.Val;
    // Now actually set the host mask
    // This will hide from the guest that we are not actually setting all of the masks it wants
    for (size_t i = 0; i < MAX_SIGNALS; ++i) {
      if (HostHandlers[i + 1].Required.load(std::memory_order_relaxed)) {
        // If it is a required host signal then we can't mask it
        HostMask &= ~(1ULL << i);
      }
    }

    ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &HostMask, nullptr, 8);
  }

  if (!!oldset) {
    *oldset = OldSet;
  }

  CheckForPendingSignals(GetTLSThread()->Thread);

  return 0;
}

uint64_t SignalDelegator::GuestSigPending(uint64_t* set, size_t sigsetsize) {
  if (sigsetsize > sizeof(uint64_t)) {
    return -EINVAL;
  }

  *set = ThreadData.PendingSignals;

  sigset_t HostSet {};
  if (sigpending(&HostSet) == 0) {
    uint64_t HostSignals {};
    for (size_t i = 0; i < MAX_SIGNALS; ++i) {
      if (sigismember(&HostSet, i + 1)) {
        HostSignals |= (1ULL << i);
      }
    }

    // Merge the real pending signal mask as well
    *set |= HostSignals;
  }
  return 0;
}

uint64_t SignalDelegator::GuestSigSuspend(uint64_t* set, size_t sigsetsize) {
  if (sigsetsize > sizeof(uint64_t)) {
    return -EINVAL;
  }

  uint64_t IgnoredSignalsMask = ~((1ULL << (SIGKILL - 1)) | (1ULL << (SIGSTOP - 1)));

  // Backup the mask
  ThreadData.PreviousSuspendMask = ThreadData.CurrentSignalMask;
  // Set the new mask
  ThreadData.CurrentSignalMask.Val = *set & IgnoredSignalsMask;
  sigset_t HostSet {};

  sigemptyset(&HostSet);

  for (int32_t i = 0; i < MAX_SIGNALS; ++i) {
    if (*set & (1ULL << i)) {
      sigaddset(&HostSet, i + 1);
    }
  }

  // Additionally we must always listen to SIGNAL_FOR_PAUSE
  // This technically forces us in to a race but should be fine
  // SIGBUS and SIGILL can't happen so we don't need to listen for them
  // sigaddset(&HostSet, SIGNAL_FOR_PAUSE);

  // Spin this in a loop until we aren't sigsuspended
  // This can happen in the case that the guest has sent signal that we can't block
  uint64_t Result = sigsuspend(&HostSet);

  // Restore Previous signal mask we are emulating
  // XXX: Might be unsafe if the signal handler adjusted the thread's signal mask
  // But since we don't support the guest adjusting the mask through the context object
  // then this is safe-ish
  ThreadData.CurrentSignalMask = ThreadData.PreviousSuspendMask;

  CheckForPendingSignals(GetTLSThread()->Thread);

  return Result == -1 ? -errno : Result;
}

uint64_t SignalDelegator::GuestSigTimedWait(uint64_t* set, siginfo_t* info, const struct timespec* timeout, size_t sigsetsize) {
  if (sigsetsize > sizeof(uint64_t)) {
    return -EINVAL;
  }

  uint64_t Result = ::syscall(SYS_rt_sigtimedwait, set, info, timeout);

  return Result == -1 ? -errno : Result;
}

uint64_t SignalDelegator::GuestSignalFD(int fd, const uint64_t* set, size_t sigsetsize, int flags) {
  if (sigsetsize > sizeof(uint64_t)) {
    return -EINVAL;
  }

  sigset_t HostSet {};
  sigemptyset(&HostSet);

  for (size_t i = 0; i < MAX_SIGNALS; ++i) {
    if (HostHandlers[i + 1].Required.load(std::memory_order_relaxed)) {
      // For now skip our internal signals
      continue;
    }

    if (*set & (1ULL << i)) {
      sigaddset(&HostSet, i + 1);
    }
  }

  // XXX: This is a barebones implementation just to get applications that listen for SIGCHLD to work
  // In the future we need our own listern thread that forwards the result
  // Thread is necessary to prevent deadlocks for a thread that has signaled on the same thread listening to the FD and blocking is enabled
  uint64_t Result = signalfd(fd, &HostSet, flags);

  return Result == -1 ? -errno : Result;
}

fextl::unique_ptr<FEX::HLE::SignalDelegator> CreateSignalDelegator(FEXCore::Context::Context* CTX, const std::string_view ApplicationName) {
  return fextl::make_unique<FEX::HLE::SignalDelegator>(CTX, ApplicationName);
}
} // namespace FEX::HLE
