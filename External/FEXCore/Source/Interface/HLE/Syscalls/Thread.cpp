#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>

#include <limits.h>
#include <linux/futex.h>
#include <stdint.h>
#include <sched.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore::HLE {
  uint64_t Getpid(FEXCore::Core::InternalThreadState *Thread) {
    uint64_t Result = ::getpid();
    SYSCALL_ERRNO();
  }

  uint64_t Clone(FEXCore::Core::InternalThreadState *Thread, uint32_t flags, void *stack, pid_t *parent_tid, pid_t *child_tid, void *tls) {
    static std::mutex SyscallMutex;
    std::scoped_lock<std::mutex> lk(SyscallMutex);

#define FLAGPRINT(x, y) if (flags & (y)) LogMan::Msg::I("\tFlag: " #x)
    FLAGPRINT(CSIGNAL,              0x000000FF);
    FLAGPRINT(CLONE_VM,             0x00000100);
    FLAGPRINT(CLONE_FS,             0x00000200);
    FLAGPRINT(CLONE_FILES,          0x00000400);
    FLAGPRINT(CLONE_SIGHAND,        0x00000800);
    FLAGPRINT(CLONE_PTRACE,         0x00002000);
    FLAGPRINT(CLONE_VFORK,          0x00004000);
    FLAGPRINT(CLONE_PARENT,         0x00008000);
    FLAGPRINT(CLONE_THREAD,         0x00010000);
    FLAGPRINT(CLONE_NEWNS,          0x00020000);
    FLAGPRINT(CLONE_SYSVSEM,        0x00040000);
    FLAGPRINT(CLONE_SETTLS,         0x00080000);
    FLAGPRINT(CLONE_PARENT_SETTID,  0x00100000);
    FLAGPRINT(CLONE_CHILD_CLEARTID, 0x00200000);
    FLAGPRINT(CLONE_DETACHED,       0x00400000);
    FLAGPRINT(CLONE_UNTRACED,       0x00800000);
    FLAGPRINT(CLONE_CHILD_SETTID,   0x01000000);
    FLAGPRINT(CLONE_NEWCGROUP,      0x02000000);
    FLAGPRINT(CLONE_NEWUTS,         0x04000000);
    FLAGPRINT(CLONE_NEWIPC,         0x08000000);
    FLAGPRINT(CLONE_NEWUSER,        0x10000000);
    FLAGPRINT(CLONE_NEWPID,         0x20000000);
    FLAGPRINT(CLONE_NEWNET,         0x40000000);
    FLAGPRINT(CLONE_IO,             0x80000000);

    FEXCore::Core::CPUState NewThreadState{};
    // Clone copies the parent thread's state
    memcpy(&NewThreadState, &Thread->State.State, sizeof(FEXCore::Core::CPUState));

    NewThreadState.gregs[FEXCore::X86State::REG_RAX] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RBX] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RBP] = 0;
    NewThreadState.gregs[FEXCore::X86State::REG_RSP] = reinterpret_cast<uint64_t>(stack);

    if (flags & CLONE_SETTLS) {
      NewThreadState.fs = reinterpret_cast<uint64_t>(tls);
    }

    // Set us to start just after the syscall instruction
    NewThreadState.rip += 2;

    auto NewThread = Thread->CTX->CreateThread(&NewThreadState, reinterpret_cast<uint64_t>(parent_tid), reinterpret_cast<uint64_t>(child_tid));
    Thread->CTX->CopyMemoryMapping(Thread, NewThread);

    // Sets the child TID to pointer in ParentTID
    if (flags & CLONE_PARENT_SETTID) {
      *parent_tid = NewThread->State.ThreadManager.GetTID();
    }

    // Sets the child TID to the pointer in ChildTID
    if (flags & CLONE_CHILD_SETTID) {
      *child_tid = NewThread->State.ThreadManager.GetTID();
    }

    // When the thread exits, clear the child thread ID at ChildTID
    // Additionally wakeup a futex at that address
    // Address /may/ be changed with SET_TID_ADDRESS syscall
    if (flags & CLONE_CHILD_CLEARTID) {
      FEXCore::Futex *futex = new FEXCore::Futex{}; // XXX: Definitely a memory leak. When should we free this?
      futex->Addr = reinterpret_cast<std::atomic<uint32_t>*>(child_tid);
      futex->Val = NewThread->State.ThreadManager.GetTID();
      Thread->CTX->SyscallHandler->EmplaceFutex(reinterpret_cast<uint64_t>(child_tid), futex);
      NewThread->State.ThreadManager.clear_tid = true;
    }

    Thread->CTX->InitializeThread(NewThread);

    // Actually start the thread
    Thread->CTX->RunThread(NewThread);

    // Return the new threads TID
    uint64_t Result = NewThread->State.ThreadManager.GetTID();
    SYSCALL_ERRNO();
  }

  uint64_t Execve(FEXCore::Core::InternalThreadState *Thread, const char *pathname, char *const argv[], char *const envp[]) {
    // XXX: Disallow execve for now
    return -ENOEXEC;
  }

  uint64_t Exit(FEXCore::Core::InternalThreadState *Thread, int status) {
    Thread->State.RunningEvents.ShouldStop = true;
    if (Thread->State.ThreadManager.clear_tid) {
      FEXCore::Futex *futex = Thread->CTX->SyscallHandler->GetFutex(Thread->State.ThreadManager.child_tid);
      futex->Addr->store(0);
      futex->cv.notify_all();
    }
    return 0;
  }

  uint64_t Wait4(FEXCore::Core::InternalThreadState *Thread, pid_t pid, int *wstatus, int options, struct rusage *rusage) {
    uint64_t Result = ::wait4(pid, wstatus, options, rusage);
    SYSCALL_ERRNO();
  }

  uint64_t Getuid(FEXCore::Core::InternalThreadState *Thread) {
    uint64_t Result = ::getuid();
    SYSCALL_ERRNO();
  }

  uint64_t Getgid(FEXCore::Core::InternalThreadState *Thread) {
    uint64_t Result = ::getgid();
    SYSCALL_ERRNO();
  }

  uint64_t Setuid(FEXCore::Core::InternalThreadState *Thread, uid_t uid) {
    uint64_t Result = ::setuid(uid);
    SYSCALL_ERRNO();
  }

  uint64_t Setgid(FEXCore::Core::InternalThreadState *Thread, gid_t gid) {
    uint64_t Result = ::setgid(gid);
    SYSCALL_ERRNO();
  }

  uint64_t Geteuid(FEXCore::Core::InternalThreadState *Thread) {
    uint64_t Result = ::geteuid();
    SYSCALL_ERRNO();
  }

  uint64_t Getegid(FEXCore::Core::InternalThreadState *Thread) {
    uint64_t Result = ::getegid();
    SYSCALL_ERRNO();
  }

  uint64_t Setregid(FEXCore::Core::InternalThreadState *Thread, gid_t rgid, gid_t egid) {
    uint64_t Result = ::setregid(rgid, egid);
    SYSCALL_ERRNO();
  }

  uint64_t Setresuid(FEXCore::Core::InternalThreadState *Thread, uid_t ruid, uid_t euid, uid_t suid) {
    uint64_t Result = ::setresuid(ruid, euid, suid);
    SYSCALL_ERRNO();
  }

  uint64_t Getresuid(FEXCore::Core::InternalThreadState *Thread, uid_t *ruid, uid_t *euid, uid_t *suid) {
    uint64_t Result = ::getresuid(ruid, euid, suid);
    SYSCALL_ERRNO();
  }

  uint64_t Setresgid(FEXCore::Core::InternalThreadState *Thread, gid_t rgid, gid_t egid, gid_t sgid) {
    uint64_t Result = ::setresgid(rgid, egid, sgid);
    SYSCALL_ERRNO();
  }

  uint64_t Getresgid(FEXCore::Core::InternalThreadState *Thread, gid_t *rgid, gid_t *egid, gid_t *sgid) {
    uint64_t Result = ::getresgid(rgid, egid, sgid);
    SYSCALL_ERRNO();
  }

  uint64_t Prctl(FEXCore::Core::InternalThreadState *Thread, int option, unsigned long arg2, unsigned long arg3, unsigned long arg4, unsigned long arg5) {
    uint64_t Result = ::prctl(option, arg2, arg3, arg4, arg5);
    SYSCALL_ERRNO();
  }

  uint64_t Arch_Prctl(FEXCore::Core::InternalThreadState *Thread, int code, unsigned long addr) {
    uint64_t Result{};
    switch (code) {
      case 0x1001: // ARCH_SET_GS
        Thread->State.State.gs = addr;
        Result = 0;
      break;
      case 0x1002: // ARCH_SET_FS
        Thread->State.State.fs = addr;
        Result = 0;
      break;
      case 0x1003: // ARCH_GET_FS
        *reinterpret_cast<uint64_t*>(addr) = Thread->State.State.fs;
        Result = 0;
      break;
      case 0x1004: // ARCH_GET_GS
        *reinterpret_cast<uint64_t*>(addr) = Thread->State.State.gs;
        Result = 0;
      break;
      case 0x3001: // ARCH_CET_STATUS
        Result = -EINVAL; // We don't support CET, return EINVAL
      break;
      default:
        LogMan::Msg::E("Unknown prctl: 0x%x", code);
        Result = -EINVAL;
        Thread->CTX->ShouldStop = true;
      break;
    }
    SYSCALL_ERRNO();
  }
  uint64_t Gettid(FEXCore::Core::InternalThreadState *Thread) {
    uint64_t Result = ::gettid();
    SYSCALL_ERRNO();
  }

  uint64_t Futex(FEXCore::Core::InternalThreadState *Thread, int *uaddr, int futex_op, int val, const struct timespec *timeout, int *uaddr2, uint32_t val3) {
    uint64_t Result = 0;

    if (0) {
      auto WakeFutex = [&](uint64_t Address, uint32_t Value) -> uint32_t {
        FEXCore::Futex *futex = Thread->CTX->SyscallHandler->GetFutex(Address);
        if (!futex) {
          futex = new FEXCore::Futex{}; // XXX: Definitely a memory leak. When should we free this?
          futex->Addr = reinterpret_cast<std::atomic<uint32_t>*>(Address);
          futex->Val = Value;
          Thread->CTX->SyscallHandler->EmplaceFutex(Address, futex);
        }

        if (Value  == INT_MAX) {
          uint32_t PrevWaiters = futex->Waiters;
          futex->cv.notify_all();
          return PrevWaiters;
        }
        else {
          uint32_t PrevWaiters = futex->Waiters;
          for (uint64_t i = 0; i < Value; ++i)
            futex->cv.notify_one();
          return std::min(PrevWaiters, Value);
        }
      };

      uint8_t Command = futex_op & 0xF;
      switch (Command) {
        case 0:   // WAIT
        case 9: { // WAIT_BITSET
          FEXCore::Futex *futex = Thread->CTX->SyscallHandler->GetFutex(reinterpret_cast<uint64_t>(uaddr));

          if (!futex) {
            futex = new FEXCore::Futex{}; // XXX: Definitely a memory leak. When should we free this?
            futex->Addr = reinterpret_cast<std::atomic<uint32_t>*>(uaddr);
            futex->Val = val;
            Thread->CTX->SyscallHandler->EmplaceFutex(reinterpret_cast<uint64_t>(uaddr), futex);
          }
          if (futex->Addr->load() != futex->Val) {
            // Immediate check can return EAGAIN
            Result = -EAGAIN;
          }
          else
          {
            std::unique_lock<std::mutex> lk(futex->Mutex);
            futex->Waiters++;
            bool PredPassed = false;
            if (timeout) {
              if (Command == 9) {
                // WAIT_BITSET is absolute time
                auto duration = std::chrono::seconds(timeout->tv_sec) + std::chrono::nanoseconds(timeout->tv_nsec);
                auto timepoint =
                  std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>(
                    std::chrono::duration_cast<std::chrono::system_clock::duration>(duration));
                PredPassed = futex->cv.wait_until(lk, timepoint, [futex] { return futex->Addr->load() != futex->Val; });
              }
              else {
                auto duration = std::chrono::seconds(timeout->tv_sec) + std::chrono::nanoseconds(timeout->tv_nsec);
                PredPassed = futex->cv.wait_for(lk, duration, [futex] { return futex->Addr->load() != futex->Val; });
              }
              if (!PredPassed) {
                Result = -ETIMEDOUT;
              }
            }
            else {
              futex->cv.wait(lk, [futex] { return futex->Addr->load() != futex->Val; });
            }
            futex->Waiters--;
          }
          break;
        }
        case 1: { // WAKE
          Result = WakeFutex(reinterpret_cast<uint64_t>(uaddr), val);
          break;
        }
        case 5: { // WAKE_OP
          FEXCore::Futex *futex = Thread->CTX->SyscallHandler->GetFutex(reinterpret_cast<uint64_t>(uaddr2));

          if (!futex) {
            futex = new FEXCore::Futex{}; // XXX: Definitely a memory leak. When should we free this?
            futex->Addr = reinterpret_cast<std::atomic<uint32_t>*>(uaddr2);
            futex->Val = reinterpret_cast<uint64_t>(timeout); // WAKE_OP reinterprets timeout as this futex's Value
            Thread->CTX->SyscallHandler->EmplaceFutex(reinterpret_cast<uint64_t>(uaddr2), futex);
          }

          int32_t OldFutexValue = futex->Addr->load();
          int32_t op = val3 >> 28 & 0b0111;
          int32_t cmp = val3 >> 24 & 0b1111;
          bool Shift = val3 >> 28 & 0b1000;
          int32_t oparg = (val3 >> 12) & 0xFFF;
          int32_t cmparg = val3 & 0xFFF;
          if (Shift)
            oparg = 1 << oparg;

          switch (op) {
            case 0: // Set
              OldFutexValue = futex->Addr->exchange(oparg);
              break;
            case 1: // Add
              OldFutexValue = futex->Addr->fetch_add(oparg);
              break;
            case 2: // Or
              OldFutexValue = futex->Addr->fetch_or(oparg);
              break;
            case 3: // AndN
              OldFutexValue = futex->Addr->fetch_and(~oparg);
              break;
            case 4: // Xor
              OldFutexValue = futex->Addr->fetch_xor(oparg);
              break;
            default: LogMan::Msg::A("Unknown WAKE_OP: %d", op); break;
          }
          // Wake up original futex still
          uint32_t PrevWaiters = WakeFutex(reinterpret_cast<uint64_t>(uaddr), val);

          bool WakeupSecond = false;
          switch (cmp) {
            case 0: // EQ
              WakeupSecond = OldFutexValue == cmparg;
              break;
            case 1: // NE
              WakeupSecond = OldFutexValue != cmparg;
              break;
            case 2: // LT
              WakeupSecond = OldFutexValue < cmparg;
              break;
            case 3: // LE
              WakeupSecond = OldFutexValue <= cmparg;
              break;
            case 4: // GT
              WakeupSecond = OldFutexValue > cmparg;
              break;
            case 5: // GE
              WakeupSecond = OldFutexValue >= cmparg;
              break;
            default: LogMan::Msg::A("Unknown comp op: %d", cmp);
          }

          // WAKE_OP reinterprets timeout as this futex's Value
          if (WakeupSecond)
            PrevWaiters += WakeFutex(reinterpret_cast<uint64_t>(uaddr2), reinterpret_cast<uint64_t>(timeout));

          Result = PrevWaiters;

          break;
        }
        case 10: { // WAKE_BITSET
          // We don't actually support this
          // Just handle it like a WAKE but wake up everything
          FEXCore::Futex *futex = Thread->CTX->SyscallHandler->GetFutex(reinterpret_cast<uint64_t>(uaddr));
          if (!futex) {
            futex = new FEXCore::Futex{}; // XXX: Definitely a memory leak. When should we free this?
            futex->Addr = reinterpret_cast<std::atomic<uint32_t>*>(uaddr);
            futex->Val = val;
            Thread->CTX->SyscallHandler->EmplaceFutex(reinterpret_cast<uint64_t>(uaddr), futex);
          }

          Result = futex->Waiters;
          futex->cv.notify_all();
          break;
        }
        case 7: // UNLOCK_PI
          Result = -EPERM;
          break;
        default:
          LogMan::Msg::A("Unknown futex command: %d", Command);
        break;
      }
    }
    else {
      return syscall(SYS_futex,
        uaddr,
        futex_op,
        val,
        timeout,
        uaddr2,
        val3);
    }
    return Result;
  }

  uint64_t Set_tid_address(FEXCore::Core::InternalThreadState *Thread, int *tidptr) {
    FEXCore::Futex *futex = Thread->CTX->SyscallHandler->GetFutex(Thread->State.ThreadManager.child_tid);
    // If a futex for this address changes then the futex location needs to change...
    if (futex) {
      Thread->CTX->SyscallHandler->RemoveFutex(Thread->State.ThreadManager.child_tid);
      futex->Addr = reinterpret_cast<std::atomic<uint32_t>*>(tidptr);
      Thread->CTX->SyscallHandler->EmplaceFutex(reinterpret_cast<uint64_t>(tidptr), futex);
    }

    Thread->State.ThreadManager.child_tid = reinterpret_cast<uint64_t>(tidptr);
    return Thread->State.ThreadManager.GetTID();
  }

  uint64_t Exit_group(FEXCore::Core::InternalThreadState *Thread, int status) {
    return 0;
  }

  uint64_t Set_robust_list(FEXCore::Core::InternalThreadState *Thread, struct robust_list_head *head, size_t len) {
    Thread->State.ThreadManager.robust_list_head = reinterpret_cast<uint64_t>(head);
    return 0;
  }

  uint64_t Prlimit64(FEXCore::Core::InternalThreadState *Thread, pid_t pid, int resource, const struct rlimit *new_limit, struct rlimit *old_limit) {
    uint64_t Result = ::prlimit(pid, (enum __rlimit_resource)(resource), new_limit, old_limit);
    SYSCALL_ERRNO();
  }
}
