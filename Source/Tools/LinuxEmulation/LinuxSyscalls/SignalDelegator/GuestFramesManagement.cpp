// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|common
desc: Handles host -> host and host -> guest signal routing, emulates procmask & co
$end_info$
*/

#include "LinuxSyscalls/SignalDelegator.h"

#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <FEXCore/Utils/MathUtils.h>
#include <FEXCore/Utils/FPState.h>

namespace FEX::HLE {
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

  fpstate->sw_reserved.magic1 = is_avx_enabled ? FEXCore::x86_64::fpx_sw_bytes::FP_XSTATE_MAGIC : 0;
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

void SignalDelegator::RestoreFrame_x64(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* Context,
                                       FEXCore::Core::CpuStateFrame* Frame, void* ucontext) {
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

    if (SupportsAVX) {
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
    if (SupportsAVX) {
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
    if (SupportsAVX) {
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

uint64_t SignalDelegator::SetupFrame_x64(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* ContextBackup,
                                         FEXCore::Core::CpuStateFrame* Frame, int Signal, siginfo_t* HostSigInfo, void* ucontext,
                                         GuestSigAction* GuestAction, stack_t* GuestStack, uint64_t NewGuestSP, const uint32_t eflags) {

  // Back up past the redzone, which is 128bytes
  // 32-bit doesn't have a redzone
  NewGuestSP -= 128;

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

  if (SupportsAVX) {
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

  FEXCore::x86_64::ucontext_t* guest_uctx = reinterpret_cast<FEXCore::x86_64::ucontext_t*>(UContextLocation);
  siginfo_t* guest_siginfo = reinterpret_cast<siginfo_t*>(SigInfoLocation);
  // Store where the host context lives in the guest stack.
  *(uint64_t*)HostStackLocation = (uint64_t)ContextBackup;

  // We have extended float information
  guest_uctx->uc_flags = FEXCore::x86_64::UC_FP_XSTATE | FEXCore::x86_64::UC_SIGCONTEXT_SS | FEXCore::x86_64::UC_STRICT_RESTORE_SS;

  // Pointer to where the fpreg memory is
  guest_uctx->uc_mcontext.fpregs = reinterpret_cast<FEXCore::x86_64::_libc_fpstate*>(FPStateLocation);
  auto* xstate = reinterpret_cast<FEXCore::x86_64::xstate*>(FPStateLocation);
  SetXStateInfo(xstate, SupportsAVX);

  guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RIP] = ContextBackup->OriginalRIP;
  guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_EFL] = eflags;
  guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_CSGSFS] = 0;

  // aarch64 and x86_64 siginfo_t matches. We can just copy this over
  // SI_USER could also potentially have random data in it, needs to be bit perfect
  // For guest faults we don't have a real way to reconstruct state to a real guest RIP
  *guest_siginfo = *HostSigInfo;

  if (ContextBackup->FaultToTopAndGeneratedException) {
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_TRAPNO] = Frame->SynchronousFaultData.TrapNo;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_ERR] = Frame->SynchronousFaultData.err_code;

    // Overwrite si_code
    guest_siginfo->si_code = Thread->CurrentFrame->SynchronousFaultData.si_code;
    Signal = Frame->SynchronousFaultData.Signal;
  } else {
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_TRAPNO] = ConvertSignalToTrapNo(Signal, HostSigInfo);
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_ERR] = ConvertSignalToError(ucontext, Signal, HostSigInfo);
  }
  guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_OLDMASK] = 0;
  guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_CR2] = 0;

#define COPY_REG(x) guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_##x] = Frame->State.gregs[FEXCore::X86State::REG_##x];
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

  auto* fpstate = &xstate->fpstate;

  // Copy float registers
  memcpy(fpstate->_st, Frame->State.mm, sizeof(Frame->State.mm));

  if (SupportsAVX) {
    CTX->ReconstructXMMRegisters(Thread, fpstate->_xmm, xstate->ymmh.ymmh_space);
  } else {
    CTX->ReconstructXMMRegisters(Thread, fpstate->_xmm, nullptr);
  }

  // FCW store default
  fpstate->fcw = Frame->State.FCW;
  fpstate->ftw = Frame->State.AbridgedFTW;

  // Reconstruct FSW
  fpstate->fsw = (Frame->State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] << 11) |
                 (Frame->State.flags[FEXCore::X86State::X87FLAG_C0_LOC] << 8) | (Frame->State.flags[FEXCore::X86State::X87FLAG_C1_LOC] << 9) |
                 (Frame->State.flags[FEXCore::X86State::X87FLAG_C2_LOC] << 10) | (Frame->State.flags[FEXCore::X86State::X87FLAG_C3_LOC] << 14);

  // Copy over signal stack information
  guest_uctx->uc_stack.ss_flags = GuestStack->ss_flags;
  guest_uctx->uc_stack.ss_sp = GuestStack->ss_sp;
  guest_uctx->uc_stack.ss_size = GuestStack->ss_size;

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

  const uint64_t SignalReturn = reinterpret_cast<uint64_t>(VDSOPointers.VDSO_kernel_sigreturn);

  NewGuestSP -= sizeof(uint64_t);
  NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(uint64_t));

  uint64_t HostStackLocation = NewGuestSP;

  if (SupportsAVX) {
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
  SetXStateInfo(xstate, SupportsAVX);

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

  if (SupportsAVX) {
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
    LOGMAN_THROW_A_FMT(SignalReturn < 0x1'0000'0000ULL, "This needs to be below 4GB");
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

  const uint64_t SignalReturn = reinterpret_cast<uint64_t>(VDSOPointers.VDSO_kernel_rt_sigreturn);

  NewGuestSP -= sizeof(uint64_t);
  NewGuestSP = FEXCore::AlignDown(NewGuestSP, alignof(uint64_t));

  uint64_t HostStackLocation = NewGuestSP;

  if (SupportsAVX) {
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
  SetXStateInfo(xstate, SupportsAVX);

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

  if (SupportsAVX) {
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
    LOGMAN_THROW_A_FMT(SignalReturn < 0x1'0000'0000ULL, "This needs to be below 4GB");
  }

  // Support regparm=3
  Frame->State.gregs[FEXCore::X86State::REG_RAX] = Signal;
  Frame->State.gregs[FEXCore::X86State::REG_RDX] = guest_uctx->pinfo;
  Frame->State.gregs[FEXCore::X86State::REG_RCX] = guest_uctx->puc;

  return NewGuestSP;
}

} // namespace FEX::HLE
