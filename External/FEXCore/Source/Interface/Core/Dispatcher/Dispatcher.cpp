
#include "Interface/Context/Context.h"
#include "Interface/Core/ArchHelpers/MContext.h"
#include "Interface/Core/Dispatcher/Dispatcher.h"
#include "Interface/Core/X86HelperGen.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Core/UContext.h>
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

void Dispatcher::SleepThread(FEXCore::Context::Context *ctx, FEXCore::Core::CpuStateFrame *Frame) {
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

ArchHelpers::Context::ContextBackup* Dispatcher::StoreThreadState(FEXCore::Core::InternalThreadState *Thread, int Signal, void *ucontext) {
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
  NewSP = AlignDown(NewSP, 16);

  auto Context = reinterpret_cast<ArchHelpers::Context::ContextBackup*>(NewSP);
  ArchHelpers::Context::BackupContext(ucontext, Context);

  // Retain the action pointer so we can see it when we return
  Context->Signal = Signal;

  // Save guest state
  // We can't guarantee if registers are in context or host GPRs
  // So we need to save everything
  memcpy(&Context->GuestState, Thread->CurrentFrame, sizeof(FEXCore::Core::CPUState));

  // Set the new SP
  ArchHelpers::Context::SetSp(ucontext, NewSP);

  // Signal frames are only used on the interpreter
  // The JITS require the stack to be setup correctly on rt_sigreturn
  if (CTX->Config.Core() == FEXCore::Config::CONFIG_INTERPRETER) {
    SignalFrames.push(NewSP);
  }

  Context->Flags = 0;
  Context->FPStateLocation = 0;
  Context->UContextLocation = 0;
  Context->SigInfoLocation = 0;

  // Store fault to top status and then reset it
  Context->FaultToTopAndGeneratedException = Thread->CurrentFrame->SynchronousFaultData.FaultToTopAndGeneratedException;
  Thread->CurrentFrame->SynchronousFaultData.FaultToTopAndGeneratedException = false;

  return Context;
}

void Dispatcher::RestoreThreadState(FEXCore::Core::InternalThreadState *Thread, void *ucontext, RestoreType Type) {
  const bool Is64BitMode = CTX->Config.Is64BitMode;
  uint64_t OldSP{};

  if (Type == RestoreType::TYPE_PAUSE) [[unlikely]] {
    if (CTX->Config.Core() == FEXCore::Config::CONFIG_IRJIT) {
      OldSP = ArchHelpers::Context::GetSp(ucontext);
    }
    else {
      LOGMAN_THROW_A_FMT(!SignalFrames.empty(), "Trying to restore a signal frame when we don't have any");
      OldSP = SignalFrames.top();
      SignalFrames.pop();
    }
  }
  else {
    // Some fun introspection here.
    // We store a pointer to our host-stack on the guest stack.
    // We need to inspect the guest state coming in, so we can get our host stack back.
    if (Is64BitMode) {
      struct rt_frame_x64 {
        // uint64_t restorer; <-- Ret removes this entry
        uint64_t StackPointer;
        // ... Additional after
      };

      rt_frame_x64 *Frame = reinterpret_cast<rt_frame_x64*>(Thread->CurrentFrame->State.gregs[X86State::REG_RSP]);
      OldSP = Frame->StackPointer;
    }
    else {
      struct FEX_PACKED rt_frame_x32 {
        // uint32_t restorer; <-- Ret removes this entry
        uint32_t Signal;
        uint32_t SigInfoPtr;
        uint32_t UContextPtr;
        uint64_t StackPointer;
        // ... Additional after
      };

      struct FEX_PACKED frame_x32 {
        // uint32_t restorer; <-- Ret removes this entry
        uint32_t Signal;
        uint32_t UContextPtr;
        uint64_t StackPointer;
        // ... Additional after
      };

      if (Type == RestoreType::TYPE_RT) {
        rt_frame_x32 *Frame = reinterpret_cast<rt_frame_x32*>(Thread->CurrentFrame->State.gregs[X86State::REG_RSP]);
        OldSP = Frame->StackPointer;
      }
      else {
        frame_x32 *Frame = reinterpret_cast<frame_x32*>(Thread->CurrentFrame->State.gregs[X86State::REG_RSP]);
        OldSP = Frame->StackPointer;
      }
    }
  }

  const bool IsAVXEnabled = CTX->Config.EnableAVX;
  uintptr_t NewSP = OldSP;
  auto Context = reinterpret_cast<ArchHelpers::Context::ContextBackup*>(NewSP);

  // First thing, reset the guest state
  memcpy(Thread->CurrentFrame, &Context->GuestState, sizeof(FEXCore::Core::CPUState));

  // Now restore host state
  ArchHelpers::Context::RestoreContext(ucontext, Context);

  if (Context->UContextLocation) {
    auto Frame = Thread->CurrentFrame;

    if (Context->Flags &ArchHelpers::Context::ContextFlags::CONTEXT_FLAG_INJIT) {
      // XXX: Unsupported since it needs state reconstruction
      // If we are in the JIT then SRA might need to be restored to values from the context
      // We can't currently support this since it might result in tearing without real state reconstruction
    }

    if (!(Context->Flags & ArchHelpers::Context::ContextFlags::CONTEXT_FLAG_32BIT)) {
      auto *guest_uctx = reinterpret_cast<FEXCore::x86_64::ucontext_t*>(Context->UContextLocation);
      [[maybe_unused]] auto *guest_siginfo = reinterpret_cast<siginfo_t*>(Context->SigInfoLocation);

      // If the guest modified the RIP then we need to take special precautions here
      if (Context->OriginalRIP != guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RIP] ||
          Context->FaultToTopAndGeneratedException) {
        // Hack! Go back to the top of the dispatcher top
        // This is only safe inside the JIT rather than anything outside of it
        ArchHelpers::Context::SetPc(ucontext, AbsoluteLoopTopAddressFillSRA);
        // Set our state register to point to our guest thread data
        ArchHelpers::Context::SetState(ucontext, reinterpret_cast<uint64_t>(Frame));

        Frame->State.rip = guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RIP];
        // XXX: Full context setting
        uint32_t eflags = guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_EFL];
        for (size_t i = 0; i < Core::CPUState::NUM_EFLAG_BITS; ++i) {
          Frame->State.flags[i] = (eflags & (1U << i)) ? 1 : 0;
        }

        Frame->State.flags[1] = 1;
        Frame->State.flags[9] = 1;

#define COPY_REG(x) \
            Frame->State.gregs[X86State::REG_##x] = guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_##x];
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
        auto *xstate = reinterpret_cast<x86_64::xstate*>(guest_uctx->uc_mcontext.fpregs);
        auto *fpstate = &xstate->fpstate;

        // Copy float registers
        memcpy(Frame->State.mm, fpstate->_st, sizeof(Frame->State.mm));

        if (IsAVXEnabled) {
          for (size_t i = 0; i < Core::CPUState::NUM_XMMS; i++) {
            memcpy(&Frame->State.xmm.avx.data[i][0], &fpstate->_xmm[i], sizeof(__uint128_t));
          }
          for (size_t i = 0; i < Core::CPUState::NUM_XMMS; i++) {
            memcpy(&Frame->State.xmm.avx.data[i][2], &xstate->ymmh.ymmh_space[i], sizeof(__uint128_t));
          }
        } else {
          memcpy(Frame->State.xmm.sse.data, fpstate->_xmm, sizeof(Frame->State.xmm.sse.data));
        }

        // FCW store default
        Frame->State.FCW = fpstate->fcw;
        Frame->State.FTW = fpstate->ftw;

        // Deconstruct FSW
        Frame->State.flags[FEXCore::X86State::X87FLAG_C0_LOC] = (fpstate->fsw >> 8) & 1;
        Frame->State.flags[FEXCore::X86State::X87FLAG_C1_LOC] = (fpstate->fsw >> 9) & 1;
        Frame->State.flags[FEXCore::X86State::X87FLAG_C2_LOC] = (fpstate->fsw >> 10) & 1;
        Frame->State.flags[FEXCore::X86State::X87FLAG_C3_LOC] = (fpstate->fsw >> 14) & 1;
        Frame->State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] = (fpstate->fsw >> 11) & 0b111;
      }
    }
    else {
      auto *guest_uctx = reinterpret_cast<FEXCore::x86::ucontext_t*>(Context->UContextLocation);
      [[maybe_unused]] auto *guest_siginfo = reinterpret_cast<FEXCore::x86::siginfo_t*>(Context->SigInfoLocation);
      // If the guest modified the RIP then we need to take special precautions here
      if (Context->OriginalRIP != guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_EIP] ||
          Context->FaultToTopAndGeneratedException) {
        // Hack! Go back to the top of the dispatcher top
        // This is only safe inside the JIT rather than anything outside of it
        ArchHelpers::Context::SetPc(ucontext, AbsoluteLoopTopAddressFillSRA);
        // Set our state register to point to our guest thread data
        ArchHelpers::Context::SetState(ucontext, reinterpret_cast<uint64_t>(Frame));

        // XXX: Full context setting
        // First 32-bytes of flags is EFLAGS broken out
        uint32_t eflags = guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_EFL];
        for (size_t i = 0; i < Core::CPUState::NUM_EFLAG_BITS; ++i) {
          Frame->State.flags[i] = (eflags & (1U << i)) ? 1 : 0;
        }

        Frame->State.flags[1] = 1;
        Frame->State.flags[9] = 1;

        Frame->State.rip = guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_EIP];
        Frame->State.cs_idx = guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_CS];
        Frame->State.ds_idx = guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_DS];
        Frame->State.es_idx = guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_ES];
        Frame->State.fs_idx = guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_FS];
        Frame->State.gs_idx = guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_GS];
        Frame->State.ss_idx = guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_SS];

        Frame->State.cs_cached = Frame->State.gdt[Frame->State.cs_idx >> 3].base;
        Frame->State.ds_cached = Frame->State.gdt[Frame->State.ds_idx >> 3].base;
        Frame->State.es_cached = Frame->State.gdt[Frame->State.es_idx >> 3].base;
        Frame->State.fs_cached = Frame->State.gdt[Frame->State.fs_idx >> 3].base;
        Frame->State.gs_cached = Frame->State.gdt[Frame->State.gs_idx >> 3].base;
        Frame->State.ss_cached = Frame->State.gdt[Frame->State.ss_idx >> 3].base;

#define COPY_REG(x) \
      Frame->State.gregs[X86State::REG_##x] = guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_##x];
        COPY_REG(RDI);
        COPY_REG(RSI);
        COPY_REG(RBP);
        COPY_REG(RBX);
        COPY_REG(RDX);
        COPY_REG(RAX);
        COPY_REG(RCX);
        COPY_REG(RSP);
#undef COPY_REG
        auto *xstate = reinterpret_cast<x86::xstate*>(guest_uctx->uc_mcontext.fpregs);
        auto *fpstate = &xstate->fpstate;

        // Copy float registers
        for (size_t i = 0; i < Core::CPUState::NUM_MMS; ++i) {
          // 32-bit st register size is only 10 bytes. Not padded to 16byte like x86-64
          memcpy(&Frame->State.mm[i], &fpstate->_st[i], 10);
        }

        // Extended XMM state
        if (IsAVXEnabled) {
          for (size_t i = 0; i < Core::CPUState::NUM_XMMS; i++) {
            memcpy(&fpstate->_xmm[i], &Frame->State.xmm.avx.data[i][0], sizeof(__uint128_t));
          }
          for (size_t i = 0; i < Core::CPUState::NUM_XMMS; i++) {
            memcpy(&xstate->ymmh.ymmh_space[i], &Frame->State.xmm.avx.data[i][2], sizeof(__uint128_t));
          }
        } else {
          memcpy(Frame->State.xmm.sse.data, fpstate->_xmm, sizeof(Frame->State.xmm.sse.data));
        }

        // FCW store default
        Frame->State.FCW = fpstate->fcw;
        Frame->State.FTW = fpstate->ftw;

        // Deconstruct FSW
        Frame->State.flags[FEXCore::X86State::X87FLAG_C0_LOC] = (fpstate->fsw >> 8) & 1;
        Frame->State.flags[FEXCore::X86State::X87FLAG_C1_LOC] = (fpstate->fsw >> 9) & 1;
        Frame->State.flags[FEXCore::X86State::X87FLAG_C2_LOC] = (fpstate->fsw >> 10) & 1;
        Frame->State.flags[FEXCore::X86State::X87FLAG_C3_LOC] = (fpstate->fsw >> 14) & 1;
        Frame->State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] = (fpstate->fsw >> 11) & 0b111;
      }
    }
  }
}

static uint32_t ConvertSignalToTrapNo(int Signal, siginfo_t *HostSigInfo) {
  switch (Signal) {
    case SIGSEGV:
      if (HostSigInfo->si_code == SEGV_MAPERR ||
          HostSigInfo->si_code == SEGV_ACCERR) {
        // Protection fault
        return X86State::X86_TRAPNO_PF;
      }
      break;
  }

  // Unknown mapping, fall back to old behaviour and just pass signal
  return Signal;
}

static uint32_t ConvertSignalToError(int Signal, siginfo_t *HostSigInfo) {
  switch (Signal) {
    case SIGSEGV:
      if (HostSigInfo->si_code == SEGV_MAPERR ||
          HostSigInfo->si_code == SEGV_ACCERR) {
        // Protection fault
        // Always a user fault for us
        // XXX: PF_PROT and PF_WRITE
        return X86State::X86_PF_USER;
      }
      break;
  }

  // Not a page fault issue
  return 0;
}

template <typename T>
static void SetXStateInfo(T* xstate, bool is_avx_enabled) {
  auto* fpstate = &xstate->fpstate;

  fpstate->sw_reserved.magic1 = x86_64::fpx_sw_bytes::FP_XSTATE_MAGIC;
  fpstate->sw_reserved.extended_size = is_avx_enabled ? sizeof(T) : 0;

  fpstate->sw_reserved.xfeatures |= x86_64::fpx_sw_bytes::FEATURE_FP |
                                    x86_64::fpx_sw_bytes::FEATURE_SSE;
  if (is_avx_enabled) {
    fpstate->sw_reserved.xfeatures |= x86_64::fpx_sw_bytes::FEATURE_YMM;
  }

  fpstate->sw_reserved.xstate_size = fpstate->sw_reserved.extended_size;

  if (is_avx_enabled) {
    xstate->xstate_hdr.xfeatures = 0;
  }
}

bool Dispatcher::HandleGuestSignal(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack) {
  auto ContextBackup = StoreThreadState(Thread, Signal, ucontext);

  auto Frame = Thread->CurrentFrame;

  // Ref count our faults
  // We use this to track if it is safe to clear cache
  ++Thread->CurrentFrame->SignalHandlerRefCounter;

  uint64_t OldPC = ArchHelpers::Context::GetPc(ucontext);
  // Set the new PC
  ArchHelpers::Context::SetPc(ucontext, AbsoluteLoopTopAddressFillSRA);
  // Set our state register to point to our guest thread data
  ArchHelpers::Context::SetState(ucontext, reinterpret_cast<uint64_t>(Frame));

  uint64_t OldGuestSP = Frame->State.gregs[X86State::REG_RSP];
  uint64_t NewGuestSP = OldGuestSP;

  // Pulling from context here
  const bool Is64BitMode = CTX->Config.Is64BitMode;
  const bool IsAVXEnabled = CTX->Config.EnableAVX;

  // Spill the SRA regardless of signal handler type
  // We are going to be returning to the top of the dispatcher which will fill again
  // Otherwise we might load garbage
  if (config.StaticRegisterAllocation) {
    if (Thread->CPUBackend->IsAddressInCodeBuffer(OldPC)) {
      uint32_t IgnoreMask{};
#ifdef _M_ARM_64
      if (Frame->InSyscallInfo != 0) {
        // We are in a syscall, this means we are in a weird register state
        // We need to spill SRA but only some of it, since some values have already been spilled
        // Lower 16 bits tells us which registers are already spilled to the context
        // So we ignore spilling those ones
        uint16_t NumRegisters = std::popcount(Frame->InSyscallInfo & 0xFFFF);
        if (NumRegisters >= 4) {
          // Unhandled case
          IgnoreMask = 0;
        }
        else {
          IgnoreMask = Frame->InSyscallInfo & 0xFFFF;
        }
      }
      else {
        // We must spill everything
        IgnoreMask = 0;
      }
#endif

      // We are in jit, SRA must be spilled
      SpillSRA(Thread, ucontext, IgnoreMask);

      ContextBackup->Flags |= ArchHelpers::Context::ContextFlags::CONTEXT_FLAG_INJIT;
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
  }

  // altstack is only used if the signal handler was setup with SA_ONSTACK
  if (GuestAction->sa_flags & SA_ONSTACK) {
    // Additionally the altstack is only used if the enabled (SS_DISABLE flag is not set)
    if (!(GuestStack->ss_flags & SS_DISABLE)) {
      // If our guest is already inside of the alternative stack
      // Then that means we are hitting recursive signals and we need to walk back the stack correctly
      uint64_t AltStackBase = reinterpret_cast<uint64_t>(GuestStack->ss_sp);
      uint64_t AltStackEnd = AltStackBase + GuestStack->ss_size;
      if (OldGuestSP >= AltStackBase &&
          OldGuestSP <= AltStackEnd) {
        // We are already in the alt stack, the rest of the code will handle adjusting this
      }
      else {
        NewGuestSP = AltStackEnd;
      }
    }
  }

  if (Is64BitMode) {
    // Back up past the redzone, which is 128bytes
    // 32-bit doesn't have a redzone
    NewGuestSP -= 128;
  }

  // siginfo_t
  siginfo_t *HostSigInfo = reinterpret_cast<siginfo_t*>(info);

  // Backup where we think the RIP currently is
  ContextBackup->OriginalRIP = Frame->State.rip;

  // Calculate eflags upfront.
  uint32_t eflags = 0;
  for (size_t i = 0; i < Core::CPUState::NUM_EFLAG_BITS; ++i) {
    eflags |= Frame->State.flags[i] << i;
  }

  if (Is64BitMode) {
    // On 64-bit the kernel sets up the siginfo_t and ucontext_t regardless of SA_SIGINFO set.
    // This allows the application to /always/ get the siginfo and ucontext even if it didn't set this flag.
    //
    // Signal frame layout on stack needs to be as follows
    // void* ReturnPointer
    // ucontext_t
    // siginfo_t
    // FP state
    if (IsAVXEnabled) {
      NewGuestSP -= sizeof(x86_64::xstate);
      NewGuestSP = AlignDown(NewGuestSP, alignof(x86_64::xstate));
    } else {
      NewGuestSP -= sizeof(x86_64::_libc_fpstate);
      NewGuestSP = AlignDown(NewGuestSP, alignof(x86_64::_libc_fpstate));
    }
    uint64_t FPStateLocation = NewGuestSP;

    NewGuestSP -= sizeof(siginfo_t);
    NewGuestSP = AlignDown(NewGuestSP, alignof(siginfo_t));
    uint64_t SigInfoLocation = NewGuestSP;

    NewGuestSP -= sizeof(FEXCore::x86_64::ucontext_t);
    NewGuestSP = AlignDown(NewGuestSP, alignof(FEXCore::x86_64::ucontext_t));
    uint64_t UContextLocation = NewGuestSP;

    ContextBackup->FPStateLocation = FPStateLocation;
    ContextBackup->UContextLocation = UContextLocation;
    ContextBackup->SigInfoLocation = SigInfoLocation;

    FEXCore::x86_64::ucontext_t *guest_uctx = reinterpret_cast<FEXCore::x86_64::ucontext_t*>(UContextLocation);
    siginfo_t *guest_siginfo = reinterpret_cast<siginfo_t*>(SigInfoLocation);

    // We have extended float information
    guest_uctx->uc_flags = FEXCore::x86_64::UC_FP_XSTATE |
                           FEXCore::x86_64::UC_SIGCONTEXT_SS |
                           FEXCore::x86_64::UC_STRICT_RESTORE_SS;

    // Pointer to where the fpreg memory is
    guest_uctx->uc_mcontext.fpregs = reinterpret_cast<x86_64::_libc_fpstate*>(FPStateLocation);
    auto *xstate = reinterpret_cast<x86_64::xstate*>(FPStateLocation);
    SetXStateInfo(xstate, IsAVXEnabled);

    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_RIP] = Frame->State.rip;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_EFL] = eflags;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_CSGSFS] = 0;

    // aarch64 and x86_64 siginfo_t matches. We can just copy this over
    // SI_USER could also potentially have random data in it, needs to be bit perfect
    // For guest faults we don't have a real way to reconstruct state to a real guest RIP
    *guest_siginfo = *HostSigInfo;
    if (ContextBackup->FaultToTopAndGeneratedException) {
      // Overwrite si_code
      guest_siginfo->si_code = Thread->CurrentFrame->SynchronousFaultData.si_code;
    }

    if (ContextBackup->FaultToTopAndGeneratedException) {
      guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_TRAPNO] = Frame->SynchronousFaultData.TrapNo;
      guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_ERR] = Frame->SynchronousFaultData.err_code;

      Signal = Frame->SynchronousFaultData.Signal;
    }
    else {
      guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_TRAPNO] = ConvertSignalToTrapNo(Signal, HostSigInfo);
      guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_ERR] = ConvertSignalToError(Signal, HostSigInfo);
    }
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_OLDMASK] = 0;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_CR2] = 0;

#define COPY_REG(x) \
    guest_uctx->uc_mcontext.gregs[FEXCore::x86_64::FEX_REG_##x] = Frame->State.gregs[X86State::REG_##x];
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

    if (IsAVXEnabled) {
      for (size_t i = 0; i < Core::CPUState::NUM_XMMS; i++) {
        memcpy(&fpstate->_xmm[i], &Frame->State.xmm.avx.data[i][0], sizeof(__uint128_t));
      }
      for (size_t i = 0; i < Core::CPUState::NUM_XMMS; i++) {
        memcpy(&xstate->ymmh.ymmh_space[i], &Frame->State.xmm.avx.data[i][2], sizeof(__uint128_t));
      }
    } else {
      memcpy(fpstate->_xmm, Frame->State.xmm.sse.data, sizeof(Frame->State.xmm.sse.data));
    }

    // FCW store default
    fpstate->fcw = Frame->State.FCW;
    fpstate->ftw = Frame->State.FTW;

    // Reconstruct FSW
    fpstate->fsw =
      (Frame->State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] << 11) |
      (Frame->State.flags[FEXCore::X86State::X87FLAG_C0_LOC] << 8) |
      (Frame->State.flags[FEXCore::X86State::X87FLAG_C1_LOC] << 9) |
      (Frame->State.flags[FEXCore::X86State::X87FLAG_C2_LOC] << 10) |
      (Frame->State.flags[FEXCore::X86State::X87FLAG_C3_LOC] << 14);

    // Copy over signal stack information
    guest_uctx->uc_stack.ss_flags = GuestStack->ss_flags;
    guest_uctx->uc_stack.ss_sp = GuestStack->ss_sp;
    guest_uctx->uc_stack.ss_size = GuestStack->ss_size;

    // Apparently RAX is always set to zero in case of badly misbehaving C applications and variadics.
    Frame->State.gregs[X86State::REG_RAX] = 0;
    Frame->State.gregs[X86State::REG_RDI] = Signal;
    Frame->State.gregs[X86State::REG_RSI] = SigInfoLocation;
    Frame->State.gregs[X86State::REG_RDX] = UContextLocation;

    // Store the host stack context backup directly in the guest stack.
    // This allows us to restore host state if we come back through rt_sigreturn.
    NewGuestSP -= 8;
    *(uint64_t*)NewGuestSP = (uint64_t)ContextBackup;

    // Host is required to provide us a restorer.
    // if the guest didn't provide a restorer then the application fails with a SIGSEGV.
    // TODO: Emulate the SIGSEGV when guest doesn't provide a restorer.
    NewGuestSP -= 8;
    *(uint64_t*)NewGuestSP = (uint64_t)GuestAction->restorer;
  }
  else {
    ContextBackup->Flags |= ArchHelpers::Context::ContextFlags::CONTEXT_FLAG_32BIT;

    if (IsAVXEnabled) {
      NewGuestSP -= sizeof(x86::xstate);
      NewGuestSP = AlignDown(NewGuestSP, alignof(x86::xstate));
    } else {
      NewGuestSP -= sizeof(x86::_libc_fpstate);
      NewGuestSP = AlignDown(NewGuestSP, alignof(x86::_libc_fpstate));
    }
    uint64_t FPStateLocation = NewGuestSP;

    NewGuestSP -= sizeof(FEXCore::x86::ucontext_t);
    NewGuestSP = AlignDown(NewGuestSP, alignof(FEXCore::x86::ucontext_t));
    uint64_t UContextLocation = NewGuestSP;

    uint64_t SigInfoLocation{};
    FEXCore::x86::siginfo_t *guest_siginfo{};
    if (GuestAction->sa_flags & SA_SIGINFO) {
      NewGuestSP -= sizeof(FEXCore::x86::siginfo_t);
      NewGuestSP = AlignDown(NewGuestSP, alignof(FEXCore::x86::siginfo_t));
      SigInfoLocation = NewGuestSP;
      guest_siginfo = reinterpret_cast<FEXCore::x86::siginfo_t*>(SigInfoLocation);
    }

    ContextBackup->FPStateLocation = FPStateLocation;
    ContextBackup->UContextLocation = UContextLocation;
    ContextBackup->SigInfoLocation = SigInfoLocation;

    FEXCore::x86::ucontext_t *guest_uctx = reinterpret_cast<FEXCore::x86::ucontext_t*>(UContextLocation);

    // We have extended float information
    guest_uctx->uc_flags = FEXCore::x86::UC_FP_XSTATE;

    // Pointer to where the fpreg memory is
    guest_uctx->uc_mcontext.fpregs = static_cast<uint32_t>(FPStateLocation);
    auto *xstate = reinterpret_cast<x86::xstate*>(FPStateLocation);
    SetXStateInfo(xstate, IsAVXEnabled);

    guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_CS] = Frame->State.cs_idx;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_DS] = Frame->State.ds_idx;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_ES] = Frame->State.es_idx;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_FS] = Frame->State.fs_idx;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_GS] = Frame->State.gs_idx;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_SS] = Frame->State.ss_idx;

    if (ContextBackup->FaultToTopAndGeneratedException) {
      guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_TRAPNO] = Frame->SynchronousFaultData.TrapNo;
      guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_ERR] = Frame->SynchronousFaultData.err_code;
      Signal = Frame->SynchronousFaultData.Signal;
    }
    else {
      guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_TRAPNO] = ConvertSignalToTrapNo(Signal, HostSigInfo);
      guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_ERR] = ConvertSignalToError(Signal, HostSigInfo);
    }
    guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_EIP] = Frame->State.rip;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_EFL] = eflags;
    guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_UESP] = 0;

#define COPY_REG(x) \
    guest_uctx->uc_mcontext.gregs[FEXCore::x86::FEX_REG_##x] = Frame->State.gregs[X86State::REG_##x];
    COPY_REG(RDI);
    COPY_REG(RSI);
    COPY_REG(RBP);
    COPY_REG(RBX);
    COPY_REG(RDX);
    COPY_REG(RAX);
    COPY_REG(RCX);
    COPY_REG(RSP);
#undef COPY_REG

    auto *fpstate = &xstate->fpstate;

    // Copy float registers
    for (size_t i = 0; i < Core::CPUState::NUM_MMS; ++i) {
      // 32-bit st register size is only 10 bytes. Not padded to 16byte like x86-64
      memcpy(&fpstate->_st[i], &Frame->State.mm[i], 10);
    }

    // Extended XMM state
    fpstate->status = FEXCore::x86::fpstate_magic::MAGIC_XFPSTATE;
    if (IsAVXEnabled) {
      for (size_t i = 0; i < std::size(Frame->State.xmm.avx.data); i++) {
        memcpy(&fpstate->_xmm[i], &Frame->State.xmm.avx.data[i][0], sizeof(__uint128_t));
      }
      for (size_t i = 0; i < std::size(Frame->State.xmm.avx.data); i++) {
        memcpy(&xstate->ymmh.ymmh_space[i], &Frame->State.xmm.avx.data[i][2], sizeof(__uint128_t));
      }
    } else {
      memcpy(fpstate->_xmm, Frame->State.xmm.sse.data, sizeof(Frame->State.xmm.sse.data));
    }

    // FCW store default
    fpstate->fcw = Frame->State.FCW;
    fpstate->ftw = Frame->State.FTW;
    // Reconstruct FSW
    fpstate->fsw =
      (Frame->State.flags[FEXCore::X86State::X87FLAG_TOP_LOC] << 11) |
      (Frame->State.flags[FEXCore::X86State::X87FLAG_C0_LOC] << 8) |
      (Frame->State.flags[FEXCore::X86State::X87FLAG_C1_LOC] << 9) |
      (Frame->State.flags[FEXCore::X86State::X87FLAG_C2_LOC] << 10) |
      (Frame->State.flags[FEXCore::X86State::X87FLAG_C3_LOC] << 14);

    // Copy over signal stack information
    guest_uctx->uc_stack.ss_flags = GuestStack->ss_flags;
    guest_uctx->uc_stack.ss_sp = static_cast<uint32_t>(reinterpret_cast<uint64_t>(GuestStack->ss_sp));
    guest_uctx->uc_stack.ss_size = GuestStack->ss_size;

    if (GuestAction->sa_flags & SA_SIGINFO) {
      if (ContextBackup->FaultToTopAndGeneratedException) {
        guest_siginfo->si_code = Frame->SynchronousFaultData.si_code;
      }
      else {
        guest_siginfo->si_code = HostSigInfo->si_code;
      }

      // These three elements are in every siginfo
      guest_siginfo->si_signo = HostSigInfo->si_signo;
      guest_siginfo->si_errno = HostSigInfo->si_errno;

      switch (Signal) {
        case SIGSEGV:
        case SIGBUS:
          // Macro expansion to get the si_addr
          // This is the address trying to be accessed, not the RIP
          guest_siginfo->_sifields._sigfault.addr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(HostSigInfo->si_addr));
          break;
        case SIGFPE:
        case SIGILL:
          // Macro expansion to get the si_addr
          // Can't really give a real result here. Pull from the context for now
          guest_siginfo->_sifields._sigfault.addr = Frame->State.rip;
          break;
        case SIGCHLD:
          guest_siginfo->_sifields._sigchld.pid = HostSigInfo->si_pid;
          guest_siginfo->_sifields._sigchld.uid = HostSigInfo->si_uid;
          guest_siginfo->_sifields._sigchld.status = HostSigInfo->si_status;
          guest_siginfo->_sifields._sigchld.utime = HostSigInfo->si_utime;
          guest_siginfo->_sifields._sigchld.stime = HostSigInfo->si_stime;
          break;
        case SIGALRM:
        case SIGVTALRM:
          guest_siginfo->_sifields._timer.tid = HostSigInfo->si_timerid;
          guest_siginfo->_sifields._timer.overrun = HostSigInfo->si_overrun;
          guest_siginfo->_sifields._timer.sigval.sival_int = HostSigInfo->si_int;
          break;
        default:
          LogMan::Msg::EFmt("Unhandled siginfo_t for signal: {}\n", Signal);
          break;
      }
    }

    // Store the host stack context backup directly in the guest stack.
    // This allows us to restore host state if we come back through rt_sigreturn.
    NewGuestSP -= 8;
    *(uint64_t*)NewGuestSP = (uint64_t)ContextBackup;

    NewGuestSP -= 4;
    *(uint32_t*)NewGuestSP = UContextLocation;

    if (GuestAction->sa_flags & SA_SIGINFO) {
      NewGuestSP -= 4;
      *(uint32_t*)NewGuestSP = SigInfoLocation;
    }
    NewGuestSP -= 4;
    *(uint32_t*)NewGuestSP = Signal;

    NewGuestSP -= 4;

    // Guest can provide its own restorer or we need to provide our own.
    // On a real host this restorer will live in VDSO.
    if (GuestAction->restorer) {
      *(uint32_t*)NewGuestSP = (uint32_t)(uint64_t)GuestAction->restorer;
    }
    else {
      const uint32_t SignalReturn = CTX->X86CodeGen.x32_rt_sigreturn;
      *(uint32_t*)NewGuestSP = SignalReturn;
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
  Frame->State.FTW = 0xFFFF;

  return true;
}

bool Dispatcher::HandleSIGILL(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) {

  if (ArchHelpers::Context::GetPc(ucontext) == SignalHandlerReturnAddressRT ||
      ArchHelpers::Context::GetPc(ucontext) == SignalHandlerReturnAddress) {
    RestoreThreadState(Thread, ucontext,
      ArchHelpers::Context::GetPc(ucontext) == SignalHandlerReturnAddressRT ? RestoreType::TYPE_RT : RestoreType::TYPE_NONRT);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --Thread->CurrentFrame->SignalHandlerRefCounter;
    return true;
  }

  if (ArchHelpers::Context::GetPc(ucontext) == PauseReturnInstruction) {
    RestoreThreadState(Thread, ucontext, RestoreType::TYPE_PAUSE);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --Thread->CurrentFrame->SignalHandlerRefCounter;
    return true;
  }

  return false;
}

bool Dispatcher::HandleSignalPause(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext) {
  FEXCore::Core::SignalEvent SignalReason = Thread->SignalReason.load();
  auto Frame = Thread->CurrentFrame;

  if (SignalReason == FEXCore::Core::SignalEvent::Pause) {
    // Store our thread state so we can come back to this
    StoreThreadState(Thread, Signal, ucontext);

    if (config.StaticRegisterAllocation && Thread->CPUBackend->IsAddressInCodeBuffer(ArchHelpers::Context::GetPc(ucontext))) {
      // We are in jit, SRA must be spilled
      ArchHelpers::Context::SetPc(ucontext, ThreadPauseHandlerAddressSpillSRA);
    } else {
      if (config.StaticRegisterAllocation) {
        // We are in non-jit, SRA is already spilled
        LOGMAN_THROW_A_FMT(!IsAddressInDispatcher(ArchHelpers::Context::GetPc(ucontext)),
                           "Signals in dispatcher have unsynchronized context");
      }
      ArchHelpers::Context::SetPc(ucontext, ThreadPauseHandlerAddress);
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
    if (config.StaticRegisterAllocation && Thread->CPUBackend->IsAddressInCodeBuffer(ArchHelpers::Context::GetPc(ucontext))) {
      // We are in jit, SRA must be spilled
      ArchHelpers::Context::SetPc(ucontext, ThreadStopHandlerAddressSpillSRA);
    } else {
      if (config.StaticRegisterAllocation) {
        // We are in non-jit, SRA is already spilled
        LOGMAN_THROW_A_FMT(!IsAddressInDispatcher(ArchHelpers::Context::GetPc(ucontext)),
                           "Signals in dispatcher have unsynchronized context");
      }
      ArchHelpers::Context::SetPc(ucontext, ThreadStopHandlerAddress);
    }

    // We need to be a little bit careful here
    // If we were already paused (due to GDB) and we are immediately stopping (due to gdb kill)
    // Then we need to ensure we don't double decrement our idle thread counter
    if (Thread->RunningEvents.ThreadSleeping) {
      // If the thread was sleeping then its idle counter was decremented
      // Reincrement it here to not break logic
      ++Thread->CTX->IdleWaitRefCount;
    }

    Thread->SignalReason.store(FEXCore::Core::SignalEvent::Nothing);
    return true;
  }

  return false;
}

uint64_t Dispatcher::GetCompileBlockPtr() {
  using ClassPtrType = void (FEXCore::Context::Context::*)(FEXCore::Core::CpuStateFrame *, uint64_t);
  union PtrCast {
    ClassPtrType ClassPtr;
    uintptr_t Data;
  };

  PtrCast CompileBlockPtr;
  CompileBlockPtr.ClassPtr = &FEXCore::Context::Context::CompileBlockJit;
  return CompileBlockPtr.Data;
}

}
