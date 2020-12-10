#include <FEXCore/Utils/LogManager.h>
#include "Common/MathUtils.h"

#include "Interface/Context/Context.h"
#include "Interface/Core/InternalThreadState.h"
#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x64/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"

#include <FEXCore/Core/X86Enums.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/futex.h>
#include <numaif.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/random.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <unistd.h>

constexpr uint64_t PAGE_SIZE = 4096;

#define BRK_SIZE 0x1000'0000

namespace FEXCore {
uint64_t SyscallHandler::HandleBRK(FEXCore::Core::InternalThreadState *Thread, void *Addr) {
  std::lock_guard<std::mutex> lk(MMapMutex);
  uint64_t Result;

  if (DataSpace == 0) {
    DefaultProgramBreak(Thread);
  }

  if (Addr == nullptr) { // Just wants to get the location of the program break atm
    Result = DataSpace + DataSpaceSize;
  }
  else {
    // Allocating out data space
    uint64_t NewEnd = reinterpret_cast<uint64_t>(Addr);
    if (NewEnd < DataSpace) {
      // Not allowed to move brk end below original start
      // Set the size to zero
      DataSpaceSize = 0;
    }
    else {
      uint64_t NewSize = NewEnd - DataSpace;

      // make sure we don't overflow to TLS storage
      if (NewSize >= BRK_SIZE)
        return -ENOMEM;

      DataSpaceSize = NewSize;
    }
    Result = DataSpace + DataSpaceSize;
  }
  return Result;
}

void SyscallHandler::DefaultProgramBreak(FEXCore::Core::InternalThreadState *Thread) {
  DataSpaceSize = 0;
  DataSpace = (uint64_t)mmap(nullptr, BRK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

SyscallHandler::SyscallHandler(FEXCore::Context::Context *ctx)
  : FM {ctx}
  , CTX {ctx} {
}

SyscallHandler *CreateHandler(Context::OperatingMode Mode, FEXCore::Context::Context *ctx) {
  if (Mode == Context::MODE_64BIT) {
    return FEXCore::HLE::x64::CreateHandler(ctx);
  }
  else {
    return FEXCore::HLE::x32::CreateHandler(ctx);
  }
}

uint64_t HandleSyscall(SyscallHandler *Handler, FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) {
  uint64_t Result{};
  Result = Handler->HandleSyscall(Thread, Args);
#ifdef DEBUG_STRACE
  Handler->Strace(Args, Result);
#endif
  return Result;
}

}
