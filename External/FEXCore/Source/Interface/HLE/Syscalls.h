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
  struct __attribute__((packed)) guest_stat {
    __kernel_ulong_t  st_dev;
    __kernel_ulong_t  st_ino;
    __kernel_ulong_t  st_nlink;

    unsigned int    st_mode;
    unsigned int    st_uid;
    unsigned int    st_gid;
    unsigned int    __pad0;
    __kernel_ulong_t  st_rdev;
    __kernel_long_t   st_size;
    __kernel_long_t   st_blksize;
    __kernel_long_t   st_blocks;  /* Number 512-byte blocks allocated. */

    __kernel_ulong_t  st_atime_;
    __kernel_ulong_t  st_atime_nsec;
    __kernel_ulong_t  st_mtime_;
    __kernel_ulong_t  st_mtime_nsec;
    __kernel_ulong_t  st_ctime_;
    __kernel_ulong_t  st_ctime_nsec;
    __kernel_long_t   __unused[3];
  };

// #define DEBUG_STRACE
class SyscallHandler {
public:
  SyscallHandler(FEXCore::Context::Context *ctx);
  virtual ~SyscallHandler() = default;

  virtual uint64_t HandleSyscall(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) = 0;

  void DefaultProgramBreak(FEXCore::Core::InternalThreadState *Thread, uint64_t Addr);

  void SetFilename(std::string const &File) { FM.SetFilename(File); }
  std::string const & GetFilename() const { return FM.GetFilename(); }

  using SyscallPtrArg0 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread);
  using SyscallPtrArg1 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t);
  using SyscallPtrArg2 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t, uint64_t);
  using SyscallPtrArg3 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg4 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg5 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
  using SyscallPtrArg6 = uint64_t(*)(FEXCore::Core::InternalThreadState *Thread, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

  struct SyscallFunctionDefinition {
    uint8_t NumArgs;
    union {
      void* Ptr;
      SyscallPtrArg0 Ptr0;
      SyscallPtrArg1 Ptr1;
      SyscallPtrArg2 Ptr2;
      SyscallPtrArg3 Ptr3;
      SyscallPtrArg4 Ptr4;
      SyscallPtrArg5 Ptr5;
      SyscallPtrArg6 Ptr6;
    };
  };

  SyscallFunctionDefinition const *GetDefinition(uint64_t Syscall) {
    return &Definitions.at(Syscall);
  }

  uint64_t HandleBRK(FEXCore::Core::InternalThreadState *Thread, void *Addr);
  uint64_t HandleMMAP(FEXCore::Core::InternalThreadState *Thread, void *addr, size_t length, int prot, int flags, int fd, off_t offset);

  FileManager FM;

#ifdef DEBUG_STRACE
  virtual void Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret) = 0;
#endif

protected:
  std::vector<SyscallFunctionDefinition> Definitions;
  std::mutex MMapMutex;

  // BRK management
  uint64_t DataSpace {};
  uint64_t DataSpaceSize {};
  uint64_t DefaultProgramBreakAddress {};

  // MMap management
  uint64_t LastMMAP = 0x1'0000'0000;
  uint64_t ENDMMAP  = 0x2'0000'0000;

private:

  FEXCore::Context::Context *CTX;

  std::mutex FutexMutex;
  std::mutex SyscallMutex;
};

enum OperatingMode {
  MODE_32BIT,
  MODE_64BIT,
};

SyscallHandler *CreateHandler(OperatingMode Mode, FEXCore::Context::Context *ctx);
uint64_t HandleSyscall(SyscallHandler *Handler, FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args);

#define SYSCALL_ERRNO() do { if (Result == -1) return -errno; return Result; } while(0)
#define SYSCALL_ERRNO_NULL() do { if (Result == 0) return -errno; return Result; } while(0)
}
