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
#include <FEXCore/Utils/Profiler.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <atomic>
#include <string.h>

#include <errno.h>
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

static FEX::HLE::ThreadStateObject* GetThreadFromAltStack(const stack_t& alt_stack) {
  // The thread object lives just before the alt-stack begin.
  FEX::HLE::ThreadStateObject* ThreadObject {};
  memcpy(&ThreadObject, reinterpret_cast<void*>(reinterpret_cast<uint64_t>(alt_stack.ss_sp) - 8), sizeof(void*));
  return ThreadObject;
}

static void SignalHandlerThunk(int Signal, siginfo_t* Info, void* UContext) {
  ucontext_t* _context = (ucontext_t*)UContext;
  auto ThreadObject = GetThreadFromAltStack(_context->uc_stack);
  FEXCORE_PROFILE_ACCUMULATION(ThreadObject->Thread, AccumulatedSignalTime);
  ThreadObject->SignalInfo.Delegator->HandleSignal(ThreadObject, Signal, Info, UContext);
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

void SignalDelegator::HandleSignal(FEX::HLE::ThreadStateObject* Thread, int Signal, void* Info, void* UContext) {
  // Let the host take first stab at handling the signal
  if (!Thread) {
    LogMan::Msg::AFmt("Thread {} has received a signal and hasn't registered itself with the delegate! Programming error!",
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
    HandleGuestSignal(Thread, Signal, Info, UContext);
  }
}

void SignalDelegator::RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
  SetHostSignalHandler(Signal, std::move(Func), Required);
  FrontendRegisterHostSignalHandler(Signal, Required);
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

  if (SupportsAVX) {
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

      if (SupportsAVX) {
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

        if (SupportsAVX) {
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

        if (SupportsAVX) {
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

  // Linux clears DF, RF, and TF flags on signal.
  Frame->State.flags[FEXCore::X86State::RFLAG_DF_RAW_LOC] = 1;
  Frame->State.flags[FEXCore::X86State::RFLAG_RF_LOC] = 0;
  Frame->State.flags[FEXCore::X86State::RFLAG_TF_RAW_LOC] = 0;

  // The guest starts its signal frame with a zero initialized FPU
  // Set that up now. Little bit costly but it's a requirement
  // This state will be restored on rt_sigreturn
  memset(Frame->State.xmm.avx.data, 0, sizeof(Frame->State.xmm));
  memset(Frame->State.mm, 0, sizeof(Frame->State.mm));
  Frame->State.FCW = 0x37F;
  Frame->State.AbridgedFTW = 0;

  // Set the new PC
  ArchHelpers::Context::SetPc(ucontext, Config.AbsoluteLoopTopAddressFillSRA);
  ArchHelpers::Context::SetFillSRASingleInst(ucontext, false);
  // Set our state register to point to our guest thread data
  ArchHelpers::Context::SetState(ucontext, reinterpret_cast<uint64_t>(Frame));

  return true;
}

bool SignalDelegator::HandleSIGILL(FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) {
  if (ArchHelpers::Context::GetPc(ucontext) == Config.SignalHandlerReturnAddress ||
      ArchHelpers::Context::GetPc(ucontext) == Config.SignalHandlerReturnAddressRT) {
    auto ThreadObject = FEX::HLE::ThreadManager::GetStateObjectFromFEXCoreThread(Thread);
    RestoreThreadState(Thread, ucontext,
                       ArchHelpers::Context::GetPc(ucontext) == Config.SignalHandlerReturnAddressRT ? RestoreType::TYPE_REALTIME :
                                                                                                      RestoreType::TYPE_NONREALTIME);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --Thread->CurrentFrame->SignalHandlerRefCounter;

    if (ThreadObject->SignalInfo.DeferredSignalFrames.size() != 0) {
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
  auto ThreadObject = FEX::HLE::ThreadManager::GetStateObjectFromFEXCoreThread(Thread);
  SignalEvent SignalReason = ThreadObject->SignalReason.load();
  auto Frame = Thread->CurrentFrame;

  if (SignalReason == SignalEvent::Pause) {
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

    ThreadObject->SignalReason.store(SignalEvent::Nothing);
    return true;
  }

  if (SignalReason == SignalEvent::Stop) {
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
    if (ThreadObject->ThreadSleeping) {
      // If the thread was sleeping then its idle counter was decremented
      // Reincrement it here to not break logic
      FEX::HLE::_SyscallHandler->TM.IncrementIdleRefCount();
    }

    ThreadObject->SignalReason.store(SignalEvent::Nothing);
    return true;
  }

  if (SignalReason == SignalEvent::Return || SignalReason == SignalEvent::ReturnRT) {
    RestoreThreadState(Thread, ucontext, SignalReason == SignalEvent::ReturnRT ? RestoreType::TYPE_REALTIME : RestoreType::TYPE_NONREALTIME);

    // Ref count our faults
    // We use this to track if it is safe to clear cache
    --Thread->CurrentFrame->SignalHandlerRefCounter;

    ThreadObject->SignalReason.store(SignalEvent::Nothing);
    return true;
  }
  return false;
}

void SignalDelegator::SignalThread(FEXCore::Core::InternalThreadState* Thread, SignalEvent Event) {
  auto ThreadObject = FEX::HLE::ThreadManager::GetStateObjectFromFEXCoreThread(Thread);
  ThreadObject->SignalReason.store(Event);
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

uint64_t SignalDelegator::GetNewSigMask(int Signal) const {
  const SignalHandler& Handler = HostHandlers[Signal];
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

  return NewMask;
}

void SignalDelegator::HandleGuestSignal(FEX::HLE::ThreadStateObject* ThreadObject, int Signal, void* Info, void* UContext) {
  auto Thread = ThreadObject->Thread;
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

        if (ThreadObject->SignalInfo.DeferredSignalFrames.empty()) {
          // No signals to defer. Just set the fault page back to RW and continue execution.
          // This occurs as a minor race condition between the refcount decrement and the access to the fault page.
          return;
        }

        const auto& Top = ThreadObject->SignalInfo.DeferredSignalFrames.back();
        Signal = Top.Signal;
        SigInfo = Top.Info;
        // sig mask has been updated at the defer time, recover the original mask
        memcpy(&_context->uc_sigmask, &Top.SigMask, sizeof(uint64_t));
        ThreadObject->SignalInfo.DeferredSignalFrames.pop_back();

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
    } else if (FaultSafeUserMemAccess::TryHandleSafeFault(Signal, SigInfo, UContext)) {
      ERROR_AND_DIE_FMT("Received invalid data to syscall. Crashing now!");
    } else {
      if (IsAsyncSignal(&SigInfo, Signal) && MustDeferSignal) {
        // If the signal is asynchronous (as determined by si_code) and FEX is in a state of needing
        // to defer the signal, then add the signal to the thread's signal queue.
        LOGMAN_THROW_A_FMT(ThreadObject->SignalInfo.DeferredSignalFrames.size() != ThreadObject->SignalInfo.DeferredSignalFrames.capacity(),
                           "Deferred signals vector hit "
                           "capacity size. This will "
                           "likely crash! Asserting now!");

        ThreadObject->SignalInfo.DeferredSignalFrames.emplace_back(ThreadStateObject::DeferredSignalState {
          .Info = SigInfo,
          .Signal = Signal,
          .SigMask = _context->uc_sigmask.__val[0],
        });

        uint64_t NewMask = GetNewSigMask(Signal);

        // Update our host signal mask so we don't hit race conditions with signals
        // This allows us to maintain the expected signal mask through the guest signal handling and then all the way back again
        memcpy(&_context->uc_sigmask, &NewMask, sizeof(uint64_t));

        // Now update the faulting page permissions so it will fault on write.
        mprotect(reinterpret_cast<void*>(&Thread->InterruptFaultPage), sizeof(Thread->InterruptFaultPage), PROT_NONE);

        // Postpone the remainder of signal handling logic until we process the SIGSEGV triggered by writing to InterruptFaultPage.
        return;
      }
    }
  }

  // Check for masked signals
  if (ThreadObject->SignalInfo.CurrentSignalMask.Val & (1ULL << (Signal - 1)) && IsAsyncSignal(&SigInfo, Signal)) {
    // This signal is masked, must defer until the guest updates the signal mask.
    // Add it to the pending signal list
    ThreadObject->SignalInfo.PendingSignals |= 1ULL << (Signal - 1);
    return;
  }

  // Let the host take first stab at handling the signal
  SignalHandler& Handler = HostHandlers[Signal];

  // Remove the pending signal
  ThreadObject->SignalInfo.PendingSignals &= ~(1ULL << (Signal - 1));

  // We have an emulation thread pointer, we can now modify its state
  if (Handler.GuestAction.sigaction_handler.handler == SIG_DFL) {
    if (Handler.DefaultBehaviour == DEFAULT_TERM || Handler.DefaultBehaviour == DEFAULT_COREDUMP) {
      // Let the signal fall through to the unhandled path
      // This way the parent process can know it died correctly
    }
  } else if (Handler.GuestAction.sigaction_handler.handler == SIG_IGN) {
    return;
  } else {
    if (Handler.GuestHandler &&
        Handler.GuestHandler(Thread, Signal, &SigInfo, UContext, &Handler.GuestAction, &ThreadObject->SignalInfo.GuestAltStack)) {
      uint64_t NewMask = GetNewSigMask(Signal);

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
    FEXCORE_TELEMETRY_OR(TYPE_CRASH_MASK, (1ULL << Signal));
    if (Signal == SIGSEGV && reinterpret_cast<uint64_t>(SigInfo.si_addr) >= SyscallHandler::TASK_MAX_64BIT) {
      // Tried accessing invalid non-canonical x86-64 address.
      FEXCORE_TELEMETRY_SET(TYPE_UNHANDLED_NONCANONICAL_ADDRESS, 1);
    }
    SaveTelemetry();
#endif

    FEX::HLE::_SyscallHandler->TM.CleanupForExit();

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

void SignalDelegator::QueueSignal(pid_t tgid, pid_t tid, int Signal, siginfo_t* info, bool IgnoreMask) {
  bool WasIgnored {};
  bool WasMasked {};
  SignalHandler& SignalHandler = HostHandlers[Signal];
  if (SignalHandler.GuestAction.sigaction_handler.handler == SIG_IGN && IgnoreMask) {
    ::syscall(SYS_rt_sigaction, Signal, &SignalHandler.OldAction, nullptr, 8);
    WasIgnored = true;
  }

  // Get the current host signal mask
  uint64_t ThreadSignalMask {};
  const uint64_t SignalMask = 1ULL << (Signal - 1);
  ::syscall(SYS_rt_sigprocmask, 0, nullptr, &ThreadSignalMask, 8);
  if (ThreadSignalMask & SignalMask) {
    WasMasked = true;

    // Signal currently masked, unmask
    ThreadSignalMask &= ~SignalMask;
    ::syscall(SYS_rt_sigprocmask, 0, &ThreadSignalMask, &ThreadSignalMask, 8);
  }

  ::syscall(SYSCALL_DEF(rt_tgsigqueueinfo), tgid, tid, Signal, info);

  if (WasMasked) {
    // Mask again
    ::syscall(SYS_rt_sigprocmask, 0, &ThreadSignalMask, nullptr, 8);
  }

  if (WasIgnored) {
    // Ignore again
    ::syscall(SYS_rt_sigaction, Signal, &SignalHandler.HostAction, nullptr, 8);
  }
}

SignalDelegator::SignalDelegator(FEXCore::Context::Context* _CTX, const std::string_view ApplicationName, bool SupportsAVX)
  : CTX {_CTX}
  , ApplicationName {ApplicationName}
  , SupportsAVX {SupportsAVX} {
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
        FEX::HLE::ThreadManager::GetStateObjectFromFEXCoreThread(Thread)->SignalInfo.Delegator->UninstallHostHandler(Signal);
        return true;
      }
      return false;
    },
    true);

  const auto PauseHandler = [](FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) -> bool {
    return FEX::HLE::ThreadManager::GetStateObjectFromFEXCoreThread(Thread)->SignalInfo.Delegator->HandleSignalPause(Thread, Signal, info, ucontext);
  };

  const auto GuestSignalHandler = [](FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext,
                                     GuestSigAction* GuestAction, stack_t* GuestStack) -> bool {
    return FEX::HLE::ThreadManager::GetStateObjectFromFEXCoreThread(Thread)->SignalInfo.Delegator->HandleDispatcherGuestSignal(
      Thread, Signal, info, ucontext, GuestAction, GuestStack);
  };

  const auto SigillHandler = [](FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext) -> bool {
    return FEX::HLE::ThreadManager::GetStateObjectFromFEXCoreThread(Thread)->SignalInfo.Delegator->HandleSIGILL(Thread, Signal, info, ucontext);
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

    FEXCORE_PROFILE_INSTANT_INCREMENT(Thread, AccumulatedSIGBUSCount, 1);
    const auto Delegator = FEX::HLE::ThreadManager::GetStateObjectFromFEXCoreThread(Thread)->SignalInfo.Delegator;
    const auto Result = FEXCore::ArchHelpers::Arm64::HandleUnalignedAccess(Thread, Delegator->GetUnalignedHandlerType(), PC,
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
}

void SignalDelegator::RegisterTLSState(FEX::HLE::ThreadStateObject* Thread) {
  FEXCore::Allocator::RegisterTLSData(Thread->Thread);

  Thread->SignalInfo.Delegator = this;

  // Set up our signal alternative stack
  // This is per thread rather than per signal
  Thread->SignalInfo.AltStackPtr = FEXCore::Allocator::mmap(nullptr, SIGSTKSZ * 16, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  stack_t altstack {};
  altstack.ss_sp = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(Thread->SignalInfo.AltStackPtr) + 8);
  altstack.ss_size = SIGSTKSZ * 16 - 8;
  altstack.ss_flags = 0;
  LOGMAN_THROW_A_FMT(!!altstack.ss_sp, "Couldn't allocate stack pointer");

  // Copy the thread object to the start of the alt-stack
  memcpy(Thread->SignalInfo.AltStackPtr, &Thread, sizeof(void*));

  // Protect the first page of the alt-stack for overflow protection.
  mprotect(Thread->SignalInfo.AltStackPtr, 4096, PROT_READ);

  // Register the alt stack
  const int Result = sigaltstack(&altstack, nullptr);
  if (Result == -1) {
    LogMan::Msg::EFmt("Failed to install alternative signal stack {}", strerror(errno));
  }

  // Get the current host signal mask
  ::syscall(SYS_rt_sigprocmask, 0, nullptr, &Thread->SignalInfo.CurrentSignalMask.Val, 8);

  if (Thread->Thread) {
    // Reserve a small amount of deferred signal frames. Usually the stack won't be utilized beyond
    // 1 or 2 signals but add a few more just in case.
    Thread->SignalInfo.DeferredSignalFrames.reserve(8);
  }
}

void SignalDelegator::UninstallTLSState(FEX::HLE::ThreadStateObject* Thread) {
  FEXCore::Allocator::munmap(Thread->SignalInfo.AltStackPtr, SIGSTKSZ * 16);

  Thread->SignalInfo.AltStackPtr = nullptr;

  stack_t altstack {};
  altstack.ss_flags = SS_DISABLE;

  // Uninstall the alt stack
  const int Result = sigaltstack(&altstack, nullptr);
  if (Result == -1) {
    LogMan::Msg::EFmt("Failed to uninstall alternative signal stack {}", strerror(errno));
  }

  FEXCore::Allocator::UninstallTLSData(Thread->Thread);
}

void SignalDelegator::FrontendRegisterHostSignalHandler(int Signal, bool Required) {
  // Linux signal handlers are per-process rather than per thread
  // Multiple threads could be calling in to this
  std::lock_guard lk(HostDelegatorMutex);
  HostHandlers[Signal].Required = Required;
  InstallHostThunk(Signal);
}

void SignalDelegator::FrontendRegisterFrontendHostSignalHandler(int Signal, bool Required) {
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
  SetFrontendHostSignalHandler(Signal, std::move(Func), Required);
  FrontendRegisterFrontendHostSignalHandler(Signal, Required);
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

uint64_t SignalDelegator::RegisterGuestSigAltStack(FEX::HLE::ThreadStateObject* Thread, const stack_t* ss, stack_t* old_ss) {
  bool UsingAltStack {};
  uint64_t AltStackBase = reinterpret_cast<uint64_t>(Thread->SignalInfo.GuestAltStack.ss_sp);
  uint64_t AltStackEnd = AltStackBase + Thread->SignalInfo.GuestAltStack.ss_size;
  uint64_t GuestSP = Thread->Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RSP];

  if (!(Thread->SignalInfo.GuestAltStack.ss_flags & SS_DISABLE) && GuestSP >= AltStackBase && GuestSP <= AltStackEnd) {
    UsingAltStack = true;
  }

  // If we have an old signal set then give it back
  if (old_ss) {
    *old_ss = Thread->SignalInfo.GuestAltStack;

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
      Thread->SignalInfo.GuestAltStack = *ss;
      return 0;
    }

    // stack size needs to be MINSIGSTKSZ (0x2000)
    if (ss->ss_size < X86_MINSIGSTKSZ) {
      return -ENOMEM;
    }

    Thread->SignalInfo.GuestAltStack = *ss;
  }

  return 0;
}

static void CheckForPendingSignals(const FEX::HLE::ThreadStateObject* Thread) {
  // Do we have any pending signals that became unmasked?
  uint64_t PendingSignals = ~Thread->SignalInfo.CurrentSignalMask.Val & Thread->SignalInfo.PendingSignals;
  if (PendingSignals != 0) {
    for (int i = 0; i < 64; ++i) {
      if (PendingSignals & (1ULL << i)) {
        FHU::Syscalls::tgkill(Thread->ThreadInfo.PID, Thread->ThreadInfo.TID, i + 1);
        // We might not even return here which is spooky
      }
    }
  }
}

uint64_t SignalDelegator::GuestSigProcMask(FEX::HLE::ThreadStateObject* Thread, int how, const uint64_t* set, uint64_t* oldset) {
  // The order in which we handle signal mask setting is important here
  // old and new can point to the same location in memory.
  // Even if the pointers are to same memory location, we must store the original signal mask
  // coming in to the syscall.
  // 1) Store old mask
  // 2) Set mask to new mask if exists
  // 3) Give old mask back
  auto OldSet = Thread->SignalInfo.CurrentSignalMask.Val;

  if (!!set) {
    uint64_t IgnoredSignalsMask = ~((1ULL << (SIGKILL - 1)) | (1ULL << (SIGSTOP - 1)));
    if (how == SIG_BLOCK) {
      Thread->SignalInfo.CurrentSignalMask.Val |= *set & IgnoredSignalsMask;
    } else if (how == SIG_UNBLOCK) {
      Thread->SignalInfo.CurrentSignalMask.Val &= ~(*set & IgnoredSignalsMask);
    } else if (how == SIG_SETMASK) {
      Thread->SignalInfo.CurrentSignalMask.Val = *set & IgnoredSignalsMask;
    } else {
      return -EINVAL;
    }

    uint64_t HostMask = Thread->SignalInfo.CurrentSignalMask.Val;
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

  CheckForPendingSignals(Thread);

  return 0;
}

uint64_t SignalDelegator::GuestSigPending(FEX::HLE::ThreadStateObject* Thread, uint64_t* set, size_t sigsetsize) {
  if (sigsetsize > sizeof(uint64_t)) {
    return -EINVAL;
  }

  *set = Thread->SignalInfo.PendingSignals;

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

uint64_t SignalDelegator::GuestSigSuspend(FEX::HLE::ThreadStateObject* Thread, uint64_t* set, size_t sigsetsize) {
  if (sigsetsize > sizeof(uint64_t)) {
    return -EINVAL;
  }

  uint64_t IgnoredSignalsMask = ~((1ULL << (SIGKILL - 1)) | (1ULL << (SIGSTOP - 1)));

  // Backup the mask
  Thread->SignalInfo.PreviousSuspendMask = Thread->SignalInfo.CurrentSignalMask;
  // Set the new mask
  Thread->SignalInfo.CurrentSignalMask.Val = *set & IgnoredSignalsMask;
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
  Thread->SignalInfo.CurrentSignalMask = Thread->SignalInfo.PreviousSuspendMask;

  CheckForPendingSignals(Thread);

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

fextl::unique_ptr<FEX::HLE::SignalDelegator>
CreateSignalDelegator(FEXCore::Context::Context* CTX, const std::string_view ApplicationName, bool SupportsAVX) {
  return fextl::make_unique<FEX::HLE::SignalDelegator>(CTX, ApplicationName, SupportsAVX);
}
} // namespace FEX::HLE
