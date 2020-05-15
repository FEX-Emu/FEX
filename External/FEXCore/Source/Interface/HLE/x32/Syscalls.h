#pragma once

#include "Interface/HLE/FileManagement.h"
#include <FEXCore/HLE/SyscallHandler.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>

namespace FEXCore::Core {
struct InternalThreadState;
}

namespace FEXCore {
class SyscallHandler;
}

namespace FEXCore::HLE::x32 {

///< Enum containing all supported x86 linux syscalls that we support
enum Syscalls {
  SYSCALL_EXIT            = 1,
  SYSCALL_READ            = 3,
  SYSCALL_WRITE           = 4,
  SYSCALL_CLOSE           = 6,
  SYSCALL_ACCESS          = 33,
  SYSCALL_BRK             = 45,
  SYSCALL_WRITEV          = 146,
  SYSCALL_MMAP2           = 192,
  SYSCALL_GETUID32        = 199,
  SYSCALL_GETGID32        = 200,
  SYSCALL_GETEUID32       = 201,
  SYSCALL_GETEGID32       = 202,
  SYSCALL_SETREUID32      = 203,
  SYSCALL_SETREGID32      = 204,
  SYSCALL_FCNTL64         = 221,
  SYSCALL_SET_THREAD_AREA = 243,
  SYSCALL_EXIT_GROUP      = 252,
  SYSCALL_OPENAT          = 295,
  SYSCALL_ARCH_PRCTL      = 384,
  SYSCALL_MAX             = 512,
};

FEXCore::SyscallHandler *CreateHandler(FEXCore::Context::Context *ctx);

}
