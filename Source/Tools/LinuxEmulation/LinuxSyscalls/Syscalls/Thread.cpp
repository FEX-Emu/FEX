// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-shared
$end_info$
*/

#include "CodeLoader.h"

#include "LinuxSyscalls/SignalDelegator.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/Syscalls/Thread.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x64/Thread.h"
#include "LinuxSyscalls/x32/Syscalls.h"
#include "LinuxSyscalls/x32/Thread.h"

#include <FEXCore/Core/Context.h>
#include <FEXCore/Core/X86Enums.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/IR/IR.h>

#include <FEXHeaderUtils/Syscalls.h>

#include <grp.h>
#include <limits.h>
#include <linux/futex.h>
#include <stdint.h>
#include <sched.h>
#include <sys/personality.h>
#include <sys/poll.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/fsuid.h>

ARG_TO_STR(idtype_t, "%u")

namespace FEX::HLE {

struct ExecutionThreadHandler {
  FEXCore::Context::Context* CTX;
  FEX::HLE::ThreadStateObject* Thread;
};

static void* ThreadHandler(void* Data) {
  ExecutionThreadHandler* Handler = reinterpret_cast<ExecutionThreadHandler*>(Data);
  auto CTX = Handler->CTX;
  auto Thread = Handler->Thread;
  FEXCore::Allocator::free(Handler);

  Thread->ThreadInfo.PID = ::getpid();
  Thread->ThreadInfo.TID = FHU::Syscalls::gettid();

  FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterTLSState(Thread);
  CTX->ExecutionThread(Thread->Thread);
  FEX::HLE::_SyscallHandler->GetSignalDelegator()->UninstallTLSState(Thread);
  FEX::HLE::_SyscallHandler->TM.DestroyThread(Thread);
  return nullptr;
}

FEX::HLE::ThreadStateObject* CreateNewThread(FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame, FEX::HLE::clone3_args* args) {
  uint64_t flags = args->args.flags;
  FEXCore::Core::CPUState NewThreadState {};
  // Clone copies the parent thread's state
  memcpy(&NewThreadState, Frame, sizeof(FEXCore::Core::CPUState));

  NewThreadState.gregs[FEXCore::X86State::REG_RAX] = 0;
  if (args->Type == TYPE_CLONE3) {
    // stack pointer points to the lowest address to the stack
    // set RSP to stack + size
    NewThreadState.gregs[FEXCore::X86State::REG_RSP] = args->args.stack + args->args.stack_size;
  } else {
    NewThreadState.gregs[FEXCore::X86State::REG_RSP] = args->args.stack;
  }

  auto NewThread = FEX::HLE::_SyscallHandler->TM.CreateThread(0, 0, &NewThreadState, args->args.parent_tid);

  if (FEX::HLE::_SyscallHandler->Is64BitMode()) {
    if (flags & CLONE_SETTLS) {
      x64::SetThreadArea(NewThread->Thread->CurrentFrame, reinterpret_cast<void*>(args->args.tls));
    }
    // Set us to start just after the syscall instruction
    x64::AdjustRipForNewThread(NewThread->Thread->CurrentFrame);
  } else {
    if (flags & CLONE_SETTLS) {
      x32::SetThreadArea(NewThread->Thread->CurrentFrame, reinterpret_cast<void*>(args->args.tls));
    }
    x32::AdjustRipForNewThread(NewThread->Thread->CurrentFrame);
  }

  // We need to do some post-thread creation setup.
  NewThread->Thread->StartPaused = true;

  // Initialize a new thread for execution.
  ExecutionThreadHandler* Arg = reinterpret_cast<ExecutionThreadHandler*>(FEXCore::Allocator::malloc(sizeof(ExecutionThreadHandler)));
  Arg->CTX = CTX;
  Arg->Thread = NewThread;
  NewThread->Thread->ExecutionThread = FEXCore::Threads::Thread::Create(ThreadHandler, Arg);

  // Wait for the thread to have started.
  NewThread->Thread->ThreadWaiting.Wait();

  if (FEX::HLE::_SyscallHandler->NeedXIDCheck()) {
    // The first time an application creates a thread, GLIBC installs their SETXID signal handler.
    // FEX needs to capture all signals and defer them to the guest.
    // Once FEX creates its first guest thread, overwrite the GLIBC SETXID handler *again* to ensure
    // FEX maintains control of the signal handler on this signal.
    FEX::HLE::_SyscallHandler->GetSignalDelegator()->CheckXIDHandler();
    FEX::HLE::_SyscallHandler->DisableXIDCheck();
  }

  // Return the new threads TID
  uint64_t Result = NewThread->ThreadInfo.TID;

  // Sets the child TID to pointer in ParentTID
  if (flags & CLONE_PARENT_SETTID) {
    *reinterpret_cast<pid_t*>(args->args.parent_tid) = Result;
  }

  // Sets the child TID to the pointer in ChildTID
  if (flags & CLONE_CHILD_SETTID) {
    NewThread->ThreadInfo.set_child_tid = reinterpret_cast<int32_t*>(args->args.child_tid);
    *reinterpret_cast<pid_t*>(args->args.child_tid) = Result;
  }

  // When the thread exits, clear the child thread ID at ChildTID
  // Additionally wakeup a futex at that address
  // Address /may/ be changed with SET_TID_ADDRESS syscall
  if (flags & CLONE_CHILD_CLEARTID) {
    NewThread->ThreadInfo.clear_child_tid = reinterpret_cast<int32_t*>(args->args.child_tid);
  }

  // clone3 flag
  if (flags & CLONE_PIDFD) {
    // Use pidfd_open to emulate this flag
    const int pidfd = ::syscall(SYSCALL_DEF(pidfd_open), Result, 0);
    if (Result == ~0ULL) {
      LogMan::Msg::EFmt("Couldn't get pidfd of TID {}\n", Result);
    } else {
      *reinterpret_cast<int*>(args->args.pidfd) = pidfd;
    }
  }

  FEX::HLE::_SyscallHandler->TM.TrackThread(NewThread);

  return NewThread;
}

uint64_t HandleNewClone(FEX::HLE::ThreadStateObject* Thread, FEXCore::Context::Context* CTX, FEXCore::Core::CpuStateFrame* Frame,
                        FEX::HLE::clone3_args* CloneArgs) {
  auto GuestArgs = &CloneArgs->args;
  uint64_t flags = GuestArgs->flags;
  auto NewThread = Thread;
  bool CreatedNewThreadObject {};

  if (flags & CLONE_THREAD) {
    FEXCore::Core::CPUState NewThreadState {};
    // Clone copies the parent thread's state
    memcpy(&NewThreadState, Frame, sizeof(FEXCore::Core::CPUState));

    NewThreadState.gregs[FEXCore::X86State::REG_RAX] = 0;
    if (GuestArgs->stack == 0) {
      // Copies in the original thread's stack
    } else {
      NewThreadState.gregs[FEXCore::X86State::REG_RSP] = GuestArgs->stack;
    }

    // Overwrite thread
    NewThread = FEX::HLE::_SyscallHandler->TM.CreateThread(0, 0, &NewThreadState, GuestArgs->parent_tid);

    // CLONE_PARENT_SETTID, CLONE_CHILD_SETTID, CLONE_CHILD_CLEARTID, CLONE_PIDFD will be handled by kernel
    // Call execution thread directly since we already are on the new thread
    NewThread->Thread->StartRunning.NotifyAll(); // Clear the start running flag
    CreatedNewThreadObject = true;
  } else {
    // If we don't have CLONE_THREAD then we are effectively a fork
    // Clear all the other threads that are being tracked
    // Frame->Thread is /ONLY/ safe to access when CLONE_THREAD flag is not set
    // Unlock the mutexes on both sides of the fork
    FEX::HLE::_SyscallHandler->UnlockAfterFork(Frame->Thread, true);

    ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &CloneArgs->SignalMask, nullptr, sizeof(CloneArgs->SignalMask));

    Thread->Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RAX] = 0;
    if (GuestArgs->stack == 0) {
      // Copies in the original thread's stack
    } else {
      Thread->Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RSP] = GuestArgs->stack;
    }
  }

  if (CloneArgs->Type == TYPE_CLONE3) {
    // If we are coming from a clone3 handler then we need to adjust RSP.
    Thread->Thread->CurrentFrame->State.gregs[FEXCore::X86State::REG_RSP] += CloneArgs->args.stack_size;
  }

  if (FEX::HLE::_SyscallHandler->Is64BitMode()) {
    if (flags & CLONE_SETTLS) {
      x64::SetThreadArea(NewThread->Thread->CurrentFrame, reinterpret_cast<void*>(GuestArgs->tls));
    }
    // Set us to start just after the syscall instruction
    x64::AdjustRipForNewThread(NewThread->Thread->CurrentFrame);
  } else {
    if (flags & CLONE_SETTLS) {
      x32::SetThreadArea(NewThread->Thread->CurrentFrame, reinterpret_cast<void*>(GuestArgs->tls));
    }
    x32::AdjustRipForNewThread(NewThread->Thread->CurrentFrame);
  }

  // Depending on clone settings, our TID and PID could have changed
  Thread->ThreadInfo.TID = FHU::Syscalls::gettid();
  Thread->ThreadInfo.PID = ::getpid();
  FEX::HLE::_SyscallHandler->FM.UpdatePID(Thread->ThreadInfo.PID);

  if (CreatedNewThreadObject) {
    FEX::HLE::_SyscallHandler->TM.TrackThread(Thread);
  }

  FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterTLSState(Thread);

  // Start exuting the thread directly
  // Our host clone starts in a new stack space, so it can't return back to the JIT space
  CTX->ExecutionThread(Thread->Thread);

  FEX::HLE::_SyscallHandler->GetSignalDelegator()->UninstallTLSState(Thread);

  // The rest of the context remains as is and the thread will continue executing
  return Thread->Thread->StatusCode;
}

uint64_t ForkGuest(FEXCore::Core::InternalThreadState* Thread, FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, void* stack,
                   size_t StackSize, pid_t* parent_tid, pid_t* child_tid, void* tls) {
  // Just before we fork, we lock all syscall mutexes so that both processes will end up with a locked mutex

  uint64_t Mask {~0ULL};
  ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &Mask, &Mask, sizeof(Mask));

  FEX::HLE::_SyscallHandler->LockBeforeFork(Frame->Thread);

  const bool IsVFork = flags & CLONE_VFORK;
  pid_t Result {};
  int VForkFDs[2];
  if (IsVFork) {
    // Use pipes as a mechanism for knowing when the child process is exiting.
    // FEX can't use `waitpid` for this since the child process may want to use it.
    // If we use `waitpid` then the kernel won't return the same data if asked again.
    pipe2(VForkFDs, O_CLOEXEC);

    // XXX: We don't currently support a real `vfork` as it causes problems.
    // Currently behaves like a fork (with wait after the fact), which isn't correct. Need to find where the problem is
    Result = fork();

    if (Result == 0) {
      // Close the read end of the pipe.
      // Keep the write end open so the parent can poll it.
      close(VForkFDs[0]);
    } else {
      // Close the write end of the pipe.
      close(VForkFDs[1]);
    }
  } else {
    Result = fork();
  }
  const bool IsChild = Result == 0;

  if (IsChild) {
    auto ThreadObject = static_cast<FEX::HLE::ThreadStateObject*>(Thread->FrontendPtr);
    // Unlock the mutexes on both sides of the fork
    FEX::HLE::_SyscallHandler->UnlockAfterFork(Frame->Thread, IsChild);

    ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &Mask, nullptr, sizeof(Mask));

    // Child
    // update the internal TID
    ThreadObject->ThreadInfo.TID = FHU::Syscalls::gettid();
    ThreadObject->ThreadInfo.PID = ::getpid();
    FEX::HLE::_SyscallHandler->FM.UpdatePID(ThreadObject->ThreadInfo.PID);
    ThreadObject->ThreadInfo.clear_child_tid = nullptr;

    // only a  single thread running so no need to remove anything from the thread array

    // Handle child setup now
    if (stack != nullptr) {
      // use specified stack
      Frame->State.gregs[FEXCore::X86State::REG_RSP] = reinterpret_cast<uint64_t>(stack) + StackSize;
    } else {
      // In the case of fork and nullptr stack then the child uses the same stack space as the parent
      // Same virtual address, different addressspace
    }

    if (FEX::HLE::_SyscallHandler->Is64BitMode()) {
      if (flags & CLONE_SETTLS) {
        x64::SetThreadArea(Frame, tls);
      }
    } else {
      // 32bit TLS doesn't just set the fs register
      if (flags & CLONE_SETTLS) {
        x32::SetThreadArea(Frame, tls);
      }
    }

    // Sets the child TID to the pointer in ChildTID
    if (flags & CLONE_CHILD_SETTID) {
      ThreadObject->ThreadInfo.set_child_tid = child_tid;
      *child_tid = ThreadObject->ThreadInfo.TID;
    }

    // When the thread exits, clear the child thread ID at ChildTID
    // Additionally wakeup a futex at that address
    // Address /may/ be changed with SET_TID_ADDRESS syscall
    if (flags & CLONE_CHILD_CLEARTID) {
      ThreadObject->ThreadInfo.clear_child_tid = child_tid;
    }

    // the rest of the context remains as is, this thread will keep executing
    return 0;
  } else {
    if (Result != -1) {
      if (flags & CLONE_PARENT_SETTID) {
        *parent_tid = Result;
      }
    }

    // Unlock the mutexes on both sides of the fork
    FEX::HLE::_SyscallHandler->UnlockAfterFork(Frame->Thread, IsChild);

    ::syscall(SYS_rt_sigprocmask, SIG_SETMASK, &Mask, nullptr, sizeof(Mask));

    // VFork needs the parent to wait for the child to exit.
    if (IsVFork) {
      // Wait for the read end of the pipe to close.
      pollfd PollFD {};
      PollFD.fd = VForkFDs[0];
      PollFD.events = POLLIN | POLLOUT | POLLRDHUP | POLLERR | POLLHUP | POLLNVAL;

      // Mask all signals until the child process returns.
      sigset_t SignalMask {};
      sigfillset(&SignalMask);
      while (ppoll(&PollFD, 1, nullptr, &SignalMask) == -1 && errno == EINTR)
        ;

      // Close the read end now.
      close(VForkFDs[0]);
    }

    // Parent
    SYSCALL_ERRNO();
  }
}

void RegisterThread(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  REGISTER_SYSCALL_IMPL(rt_sigreturn, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
    FEX::HLE::_SyscallHandler->GetSignalDelegator()->HandleSignalHandlerReturn(true);
    FEX_UNREACHABLE;
  });

  REGISTER_SYSCALL_IMPL_FLAGS(fork, SyscallFlags::DEFAULT, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
    return ForkGuest(Frame->Thread, Frame, 0, 0, 0, 0, 0, 0);
  });

  REGISTER_SYSCALL_IMPL_FLAGS(vfork, SyscallFlags::DEFAULT, [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
    return ForkGuest(Frame->Thread, Frame, CLONE_VFORK, 0, 0, 0, 0, 0);
  });

  REGISTER_SYSCALL_IMPL_FLAGS(getpgrp, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame) -> uint64_t {
                                uint64_t Result = ::getpgrp();
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(clone3, SyscallFlags::DEFAULT,
                              ([](FEXCore::Core::CpuStateFrame* Frame, FEX::HLE::kernel_clone3_args* cl_args, size_t size) -> uint64_t {
                                FEX::HLE::clone3_args args {};
                                args.Type = TypeOfClone::TYPE_CLONE3;
                                memcpy(&args.args, cl_args, std::min(sizeof(FEX::HLE::kernel_clone3_args), size));
                                return CloneHandler(Frame, &args);
                              }));

  REGISTER_SYSCALL_IMPL_FLAGS(exit, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY | SyscallFlags::NORETURN,
                              [](FEXCore::Core::CpuStateFrame* Frame, int status) -> uint64_t {
                                auto Thread = Frame->Thread;

                                // TLS/DTV teardown is something FEX can't control. Disable glibc checking when we leave a pthread.
                                // Since this thread is hard stopping, we can't track the TLS/DTV teardown in FEX's thread handling.
                                FEXCore::Allocator::YesIKnowImNotSupposedToUseTheGlibcAllocator::HardDisable();
                                auto ThreadObject = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);

                                if (ThreadObject->ThreadInfo.clear_child_tid) {
                                  std::atomic<uint32_t>* Addr = reinterpret_cast<std::atomic<uint32_t>*>(ThreadObject->ThreadInfo.clear_child_tid);
                                  Addr->store(0);
                                  syscall(SYSCALL_DEF(futex), ThreadObject->ThreadInfo.clear_child_tid, FUTEX_WAKE, ~0ULL, 0, 0, 0);
                                }

                                Thread->StatusCode = status;
                                FEX::HLE::_SyscallHandler->TM.StopThread(ThreadObject);

                                return 0;
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(prctl, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, int option, unsigned long arg2, unsigned long arg3,
                                 unsigned long arg4, unsigned long arg5) -> uint64_t {
                                uint64_t Result {};
#ifndef PR_GET_AUXV
#define PR_GET_AUXV 0x41555856
#endif
                                switch (option) {
                                case PR_SET_SECCOMP:
                                case PR_GET_SECCOMP:
                                  // FEX doesn't support seccomp
                                  return -EINVAL;
                                  break;
                                case PR_GET_AUXV: {
                                  if (arg4 || arg5) {
                                    return -EINVAL;
                                  }

                                  void* addr = reinterpret_cast<void*>(arg2);
                                  size_t UserSize = reinterpret_cast<size_t>(arg3);

                                  uint64_t auxvBase = 0;
                                  uint64_t auxvSize = 0;
                                  FEX::HLE::_SyscallHandler->GetCodeLoader()->GetAuxv(auxvBase, auxvSize);
                                  size_t MinSize = std::min(auxvSize, UserSize);

                                  memcpy(addr, reinterpret_cast<void*>(auxvBase), MinSize);

                                  // Returns the size of auxv without truncation.
                                  return auxvSize;
                                }
                                default: Result = ::prctl(option, arg2, arg3, arg4, arg5); break;
                                }
                                SYSCALL_ERRNO();
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(arch_prctl, SyscallFlags::DEFAULT, [](FEXCore::Core::CpuStateFrame* Frame, int code, unsigned long addr) -> uint64_t {
    uint64_t Result {};
    switch (code) {
    case 0x1001: // ARCH_SET_GS
      if (addr >= SyscallHandler::TASK_MAX_64BIT) {
        // Ignore a non-canonical address
        return -EPERM;
      }
      Frame->State.gs_cached = addr;
      Result = 0;
      break;
    case 0x1002: // ARCH_SET_FS
      if (addr >= SyscallHandler::TASK_MAX_64BIT) {
        // Ignore a non-canonical address
        return -EPERM;
      }
      Frame->State.fs_cached = addr;
      Result = 0;
      break;
    case 0x1003: // ARCH_GET_FS
      *reinterpret_cast<uint64_t*>(addr) = Frame->State.fs_cached;
      Result = 0;
      break;
    case 0x1004: // ARCH_GET_GS
      *reinterpret_cast<uint64_t*>(addr) = Frame->State.gs_cached;
      Result = 0;
      break;
    case 0x3001:        // ARCH_CET_STATUS
      Result = -EINVAL; // We don't support CET, return EINVAL
      break;
    case 0x1011: // ARCH_GET_CPUID
      return 1;
      break;
    case 0x1012:      // ARCH_SET_CPUID
      return -ENODEV; // Claim we don't support faulting on CPUID
      break;
    default:
      LogMan::Msg::EFmt("Unknown prctl: 0x{:x}", code);
      Result = -EINVAL;
      break;
    }
    SYSCALL_ERRNO();
  });

  REGISTER_SYSCALL_IMPL_FLAGS(set_tid_address, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY,
                              [](FEXCore::Core::CpuStateFrame* Frame, int* tidptr) -> uint64_t {
                                auto ThreadObject = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);
                                ThreadObject->ThreadInfo.clear_child_tid = tidptr;
                                return ThreadObject->ThreadInfo.TID;
                              });

  REGISTER_SYSCALL_IMPL_FLAGS(exit_group, SyscallFlags::OPTIMIZETHROUGH | SyscallFlags::NOSYNCSTATEONENTRY | SyscallFlags::NORETURN,
                              [](FEXCore::Core::CpuStateFrame* Frame, int status) -> uint64_t {
                                // Save telemetry if we're exiting.
                                FEX::HLE::_SyscallHandler->GetSignalDelegator()->SaveTelemetry();

                                syscall(SYSCALL_DEF(exit_group), status);
                                // This will never be reached
                                std::terminate();
                              });
}
} // namespace FEX::HLE
