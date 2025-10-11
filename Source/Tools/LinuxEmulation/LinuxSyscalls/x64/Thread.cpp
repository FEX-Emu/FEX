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

namespace FEX::HLE {
uint64_t SyscallHandler::read_ldt(FEXCore::Core::CpuStateFrame* Frame, void* ptr, unsigned long bytecount) {
  auto Thread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);

  if (!Thread->ldt_entries) {
    return 0;
  }

  bytecount = std::min(bytecount, MAX_LDT_ENTRIES * LDT_ENTRY_SIZE);
  const auto EntriesToCopySize = std::min(bytecount, Thread->ldt_entry_count * LDT_ENTRY_SIZE);

  if (FaultSafeUserMemAccess::CopyToUser(ptr, Thread->ldt_entries, EntriesToCopySize) != EntriesToCopySize) {
    return -EFAULT;
  }

  // Quirk that if the number of bytes that the user is asking for is larger than the amount we have, then zero the remaining memory.
  // This means the guest can't ever know the actual size of the LDT.
  size_t RemainingSize = bytecount - EntriesToCopySize;
  if (RemainingSize) {
    void* remaining = alloca(RemainingSize);
    memset(remaining, 0, RemainingSize);
    if (FaultSafeUserMemAccess::CopyToUser(reinterpret_cast<uint8_t*>(ptr) + EntriesToCopySize, remaining, RemainingSize) != RemainingSize) {
      return -EFAULT;
    }
  }

  // Return the combined size of ldt entries and zero initialized range.
  // I don't make the rules, it's just the weirdness that the kernel does.
  return bytecount;
}

static uint64_t read_default_ldt(FEXCore::Core::CpuStateFrame* Frame, void* ptr, unsigned long bytecount) {
  // This is some weird old legacy thing. Just returns zeroes up to 128-bytes.
  uint8_t Data[128] {};
  bytecount = std::min<uint64_t>(bytecount, sizeof(Data));

  if (FaultSafeUserMemAccess::CopyToUser(ptr, Data, bytecount) != bytecount) {
    return -EFAULT;
  }

  return bytecount;
}

uint64_t SyscallHandler::write_ldt(FEXCore::Core::CpuStateFrame* Frame, void* ptr, unsigned long bytecount, bool legacy) {
  auto Thread = FEX::HLE::ThreadManager::GetStateObjectFromCPUState(Frame);

  struct user_desc_x64 {
    uint32_t entry_number;
    uint32_t base_addr;
    uint32_t limit;
    uint32_t seg_32bit       : 1;
    uint32_t contents        : 2;
    uint32_t read_exec_only  : 1;
    uint32_t limit_in_pages  : 1;
    uint32_t seg_not_present : 1;
    uint32_t useable         : 1;
    uint32_t lm              : 1;
  };
  static_assert(sizeof(user_desc_x64) == 16);

  // `content` member variables.
  constexpr static uint32_t MODIFY_LDT_CONTENTS_CONFORMING = 3;

  if (bytecount != sizeof(user_desc_x64)) {
    // Can only write a single ldt. Reject smaller and larger values.
    return -EINVAL;
  }

  user_desc_x64 ldt_info {};
  FEXCore::Core::CPUState::gdt_segment ldt {};

  if (FaultSafeUserMemAccess::CopyFromUser(&ldt_info, ptr, sizeof(ldt_info)) == EFAULT) {
    // Reject if we can't read it.
    return -EFAULT;
  }

  if (ldt_info.entry_number > MAX_LDT_ENTRIES) {
    return -EINVAL;
  }

  if (ldt_info.contents == MODIFY_LDT_CONTENTS_CONFORMING) {
    // Conforming is mostly ignored.
    // Legacy doesn't support it at all. Good.
    if (legacy) {
      return -EINVAL;
    }
    // Non-legacy ignores if only if the `seg_not_present` is set.
    if (ldt_info.seg_not_present == 0) {
      return -EINVAL;
    }
  }

  auto is_empty = [](user_desc_x64 ldt_info, bool legacy) {
    // Legacy empty is trivial.
    const bool legacy_empty = legacy && ldt_info.base_addr == 0 && ldt_info.limit == 0;
    if (legacy_empty) {
      return true;
    }

    // Non-legacy is a bit more work.
    return ldt_info.base_addr == 0 && ldt_info.limit == 0 && ldt_info.contents == 0 && ldt_info.read_exec_only == 1 &&
           ldt_info.limit_in_pages == 0 && ldt_info.seg_not_present == 1 && ldt_info.useable == 0;
  };

  auto fill_ldt = [](FEXCore::Core::CPUState::gdt_segment& segment, user_desc_x64 ldt_info) {
    FEXCore::Core::CPUState::SetGDTBase(&segment, ldt_info.base_addr);
    FEXCore::Core::CPUState::SetGDTLimit(&segment, ldt_info.limit);

    // Additional flags
    // Type: bit [11:8]
    // - bit[8]  - Accessed
    // - bit[9]  - Readable
    // - bit[10] - Conforming
    // - bit[11]
    //   - 1 - Code
    //   - 0 - Data
    segment.Type = ((ldt_info.read_exec_only ^ 1) << 1) | // Readable
                   (ldt_info.contents << 2) |             // Code/Data+Conforming
                   1;                                     // Accessed
    // S: bit [12]
    // - 0 (System descriptor)
    // - 1 (User descriptor)
    segment.S = 1;
    // DPL: bit[14:13]
    segment.DPL = 3;
    // P: Present
    segment.P = ldt_info.seg_not_present ^ 1;
    // AVL: Available to software
    segment.AVL = ldt_info.useable;
    // L: Long-mode
    // This doesn't allow setting 64-bit segments!
    segment.L = 0;
    // D: Default operand size
    // - 0: 16-bit operand size
    // - 1: 32-bit operand size
    segment.D = ldt_info.seg_32bit;
    // G: Granularity
    segment.G = ldt_info.limit_in_pages;
  };

  if (is_empty(ldt_info, legacy)) {
    // If the ldt_info is considered empty then this is a zeroing operation.
    // Just use the zero ldt.
  } else {
    // This syscall only allows installing 32-bit segments. If `seg_32bit` isn't set then
    // it assumes a 16-bit segment!
    if (!ldt_info.seg_32bit) {
      return -EINVAL;
    }

    fill_ldt(ldt, ldt_info);

    if (legacy) {
      // Legacy always zeros this.
      ldt.AVL = 0;
    }
  }

  // Need to be careful with ldt replacement here to ensure it is atomically visible.
  auto old_ldt = Thread->ldt_entries;
  auto old_ldt_entries = Thread->ldt_entry_count;

  const auto new_ldt_count = std::max<size_t>(old_ldt_entries, ldt_info.entry_number + 1);
  const auto new_ldt_size = new_ldt_count * LDT_ENTRY_SIZE;

  const auto new_ldt_entries = reinterpret_cast<FEXCore::Core::CPUState::gdt_segment*>(
    FEXCore::Allocator::mmap(nullptr, new_ldt_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

  FEXCore::Allocator::VirtualName("FEXMem_Misc", reinterpret_cast<void*>(new_ldt_entries), new_ldt_size);

  if (old_ldt) {
    // Copy old entries if they existed.
    memcpy(new_ldt_entries, old_ldt, old_ldt_entries * LDT_ENTRY_SIZE);
  }

  // Set new LDT.
  new_ldt_entries[ldt_info.entry_number] = ldt;

  // Set new LDT pointer.
  Thread->ldt_entries = new_ldt_entries;
  Thread->ldt_entry_count = new_ldt_count;

  // Give the new LDT to CPUState.
  Frame->State.segment_arrays[FEXCore::Core::CPUState::SEGMENT_ARRAY_INDEX_LDT] = new_ldt_entries;

  if (old_ldt) {
    FEXCore::Allocator::munmap(old_ldt, old_ldt_entries * LDT_ENTRY_SIZE);
  }

  return 0;
}

} // namespace FEX::HLE

namespace FEX::HLE::x64 {
uint64_t SetThreadArea(FEXCore::Core::CpuStateFrame* Frame, void* tls) {
  Frame->State.fs_cached = reinterpret_cast<uint64_t>(tls);
  return 0;
}

void AdjustRipForNewThread(FEXCore::Core::CpuStateFrame* Frame) {
  Frame->State.rip += 2;
}

enum Modify_ldt_func : int32_t {
  LDT_READ = 0,
  LDT_WRITE_LEGACY = 1,
  LDT_READ_DEFAULT = 2,
  LDT_WRITE = 0x11,
};

void RegisterThread(FEX::HLE::SyscallHandler* Handler) {
  using namespace FEXCore::IR;
  REGISTER_SYSCALL_IMPL_X64(modify_ldt, [](FEXCore::Core::CpuStateFrame* Frame, int func, void* ptr, unsigned long bytecount) -> uint64_t {
    switch (func) {
    case Modify_ldt_func::LDT_READ: return FEX::HLE::_SyscallHandler->read_ldt(Frame, ptr, bytecount);
    case Modify_ldt_func::LDT_WRITE_LEGACY: return FEX::HLE::_SyscallHandler->write_ldt(Frame, ptr, bytecount, true);
    case Modify_ldt_func::LDT_READ_DEFAULT: return read_default_ldt(Frame, ptr, bytecount);
    case Modify_ldt_func::LDT_WRITE: return FEX::HLE::_SyscallHandler->write_ldt(Frame, ptr, bytecount, false);
    default: return -ENOSYS;
    }
  });

  REGISTER_SYSCALL_IMPL_X64(
    clone, ([](FEXCore::Core::CpuStateFrame* Frame, uint32_t flags, void* stack, pid_t* parent_tid, pid_t* child_tid, void* tls) -> uint64_t {
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

            .flags = flags & ~CSIGNAL, // This no longer contains CSIGNAL
            .pidfd = 0,                // For clone, pidfd is duplicated here
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
  REGISTER_SYSCALL_IMPL_X64(execve, [](FEXCore::Core::CpuStateFrame* Frame, const char* pathname, char* const argv[], char* const envp[]) -> uint64_t {
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

  REGISTER_SYSCALL_IMPL_X64(execveat, ([](FEXCore::Core::CpuStateFrame* Frame, int dirfd, const char* pathname, char* const argv[],
                                          char* const envp[], int flags) -> uint64_t {
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
