// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|syscalls-x86-64
$end_info$
*/

#include "LinuxSyscalls/SignalDelegator.h"
#include "LinuxSyscalls/Syscalls.h"
#include "LinuxSyscalls/x64/Syscalls.h"
#include "LinuxSyscalls/x64/Thread.h"

#include <FEXCore/Core/CoreState.h>
#include <FEXCore/Debug/InternalThreadState.h>
#include <FEXCore/IR/IR.h>
#include <FEXCore/fextl/vector.h>

#include <sched.h>
#include <signal.h>
#include <stddef.h>
#include <syscall.h>
#include <stdint.h>
#include <unistd.h>

namespace FEX::HLE::x64 {
uint64_t SetThreadArea(FEXCore::Core::CpuStateFrame* Frame, void* tls) {
  Frame->State.fs_cached = reinterpret_cast<uint64_t>(tls);
  return 0;
}

void AdjustRipForNewThread(FEXCore::Core::CpuStateFrame* Frame) {
  Frame->State.rip += 2;
}

void RegisterThread(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;

  REGISTER_SYSCALL_IMPL_X64_FLAGS(
    clone, SyscallFlags::DEFAULT,
    ([](FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, void* stack, pid_t* parent_tid, pid_t* child_tid, void* tls) -> uint64_t {
      // This is slightly different EFAULT behaviour, if child_tid or parent_tid is invalid then the kernel just doesn't write to the
      // pointer. Still need to be EFAULT safe although.
      if ((flags & (CLONE_CHILD_SETTID | CLONE_CHILD_CLEARTID)) && child_tid) {
        FaultSafeUserMemAccess::VerifyIsWritable(child_tid, sizeof(*child_tid));
      }

      if ((flags & CLONE_PARENT_SETTID) && parent_tid) {
        FaultSafeUserMemAccess::VerifyIsWritable(parent_tid, sizeof(*parent_tid));
      }

      FEX::HLE::clone3_args args {
        .Type = TypeOfClone::TYPE_CLONE2,
        .args =
          {
            .flags = flags, // CSIGNAL is contained in here
            .pidfd = 0,     // For clone, pidfd is duplicated here
            .child_tid = reinterpret_cast<uint64_t>(child_tid),
            .parent_tid = reinterpret_cast<uint64_t>(parent_tid),
            .exit_signal = flags & CSIGNAL,
            .stack = reinterpret_cast<uint64_t>(stack),
            .stack_size = 0, // This syscall isn't able to see the stack size
            .tls = reinterpret_cast<uint64_t>(tls),
            .set_tid = 0, // This syscall isn't able to select TIDs
            .set_tid_size = 0,
            .cgroup = 0, // This syscall can't select cgroups
          },
      };
      return CloneHandler(Frame, &args);
    }));

  REGISTER_SYSCALL_IMPL_X64(sigaltstack, [](FEXCore::Core::CpuStateFrame* Frame, const stack_t* ss, stack_t* old_ss) -> uint64_t {
    FaultSafeUserMemAccess::VerifyIsReadableOrNull(ss, sizeof(*ss));
    FaultSafeUserMemAccess::VerifyIsWritableOrNull(old_ss, sizeof(*old_ss));
    return FEX::HLE::_SyscallHandler->GetSignalDelegator()->RegisterGuestSigAltStack(
      FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame), ss, old_ss);
  });

  // launch a new process under fex
  // currently does not propagate argv[0] correctly
  REGISTER_SYSCALL_IMPL_X64_FLAGS(execve, SyscallFlags::DEFAULT,
                                  [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, char* const argv[], char* const envp[]) -> uint64_t {
                                    fextl::vector<const char*> Args;
                                    fextl::vector<const char*> Envp;

                                    if (argv) {
                                      for (int i = 0; argv[i]; i++) {
                                        Args.push_back(argv[i]);
                                      }

                                      Args.push_back(nullptr);
                                    }

                                    if (envp) {
                                      for (int i = 0; envp[i]; i++) {
                                        Envp.push_back(envp[i]);
                                      }

                                      Envp.push_back(nullptr);
                                    }

                                    auto* const* ArgsPtr = argv ? const_cast<char* const*>(Args.data()) : nullptr;
                                    auto* const* EnvpPtr = envp ? const_cast<char* const*>(Envp.data()) : nullptr;

                                    FEX::HLE::ExecveAtArgs AtArgs = FEX::HLE::ExecveAtArgs::Empty();

                                    return FEX::HLE::ExecveHandler(Frame, pathname, ArgsPtr, EnvpPtr, AtArgs);
                                  });

  REGISTER_SYSCALL_IMPL_X64_FLAGS(
    execveat, SyscallFlags::DEFAULT,
    ([](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, char* const argv[], char* const envp[], int flags) -> uint64_t {
      fextl::vector<const char*> Args;
      fextl::vector<const char*> Envp;

      if (argv) {
        for (int i = 0; argv[i]; i++) {
          Args.push_back(argv[i]);
        }

        Args.push_back(nullptr);
      }

      if (envp) {
        for (int i = 0; envp[i]; i++) {
          Envp.push_back(envp[i]);
        }

        Envp.push_back(nullptr);
      }

      FEX::HLE::ExecveAtArgs AtArgs {
        .dirfd = dirfd,
        .flags = flags,
      };

      auto* const* ArgsPtr = argv ? const_cast<char* const*>(Args.data()) : nullptr;
      auto* const* EnvpPtr = envp ? const_cast<char* const*>(Envp.data()) : nullptr;
      return FEX::HLE::ExecveHandler(Frame, pathname, ArgsPtr, EnvpPtr, AtArgs);
    }));
}
} // namespace FEX::HLE::x64
