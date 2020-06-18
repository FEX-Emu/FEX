#include "Interface/Context/Context.h"

#include "Interface/HLE/Syscalls.h"
#include "Interface/HLE/x32/Syscalls.h"
#include "Interface/HLE/x32/FD.h"
#include "Interface/HLE/x32/Memory.h"
#include "Interface/HLE/x32/Thread.h"

#if 0
#include "Interface/HLE/Syscalls/FS.h"
#include "Interface/HLE/Syscalls/Info.h"
#include "Interface/HLE/Syscalls/Ioctl.h"
#include "Interface/HLE/Syscalls/Memory.h"
#include "Interface/HLE/Syscalls/Numa.h"
#include "Interface/HLE/Syscalls/Sched.h"
#include "Interface/HLE/Syscalls/Semaphore.h"
#include "Interface/HLE/Syscalls/SHM.h"
#include "Interface/HLE/Syscalls/Signals.h"
#include "Interface/HLE/Syscalls/Socket.h"
#include "Interface/HLE/Syscalls/Thread.h"
#include "Interface/HLE/Syscalls/Time.h"
#include "Interface/HLE/Syscalls/Timer.h"

#endif

#include "LogManager.h"

namespace {
  uint64_t Unimplemented(FEXCore::Core::InternalThreadState *Thread) {
    ERROR_AND_DIE("Unhandled system call");
    return -1;
  }

  uint64_t NopSuccess(FEXCore::Core::InternalThreadState *Thread) {
    return 0;
  }
  uint64_t NopFail(FEXCore::Core::InternalThreadState *Thread) {
    return -1ULL;
  }
}


namespace FEXCore::HLE::x32 {

class x32SyscallHandler final : public FEXCore::SyscallHandler {
public:
  x32SyscallHandler(FEXCore::Context::Context *ctx);

  // In the case that the syscall doesn't hit the optimized path then we still need to go here
  uint64_t HandleSyscall(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) override;

#ifdef DEBUG_STRACE
  void Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret) override;
#endif

private:
  void RegisterSyscallHandlers();
};

#ifdef DEBUG_STRACE
void x32SyscallHandler::Strace(FEXCore::HLE::SyscallArguments *Args, uint64_t Ret) {
}
#endif

x32SyscallHandler::x32SyscallHandler(FEXCore::Context::Context *ctx)
  : SyscallHandler {ctx} {
  RegisterSyscallHandlers();
}

void x32SyscallHandler::RegisterSyscallHandlers() {
  Definitions.resize(FEXCore::HLE::x32::SYSCALL_MAX);
  auto cvt = [](auto in) {
    union {
      decltype(in) val;
      void *raw;
    } raw;
    raw.val = in;
    return raw.raw;
  };

  // Clear all definitions
  for (auto &Def : Definitions) {
    Def.NumArgs = 255;
    Def.Ptr = cvt(&Unimplemented);
  }

  const std::vector<std::tuple<uint16_t, void*, uint8_t>> Syscalls = {
    #if 0
    {SYSCALL_READ,                     cvt(&FEXCore::HLE::Read),                   3},
    {SYSCALL_EXIT,                     cvt(&FEXCore::HLE::Exit),                   1},
    {SYSCALL_WRITE,                    cvt(&FEXCore::HLE::Write),                  3},
    {SYSCALL_CLOSE,                    cvt(&FEXCore::HLE::Close),                  1},
    {SYSCALL_ACCESS,                   cvt(&FEXCore::HLE::Access),                 2},
    {SYSCALL_BRK,                      cvt(&FEXCore::HLE::Brk),                    1},
    {SYSCALL_WRITEV,                   cvt(&FEXCore::HLE::x32::Writev),            3},
    {SYSCALL_MMAP2,                    cvt(&FEXCore::HLE::x32::Mmap),              6},
    {SYSCALL_GETUID32,                 cvt(&FEXCore::HLE::Getuid),                 0},
    {SYSCALL_GETGID32,                 cvt(&FEXCore::HLE::Getgid),                 0},
    {SYSCALL_GETEUID32,                cvt(&FEXCore::HLE::Geteuid),                0},
    {SYSCALL_GETEGID32,                cvt(&FEXCore::HLE::Getegid),                0},
    {SYSCALL_FCNTL64,                  cvt(&FEXCore::HLE::Fcntl),                  3},
    {SYSCALL_SET_THREAD_AREA,          cvt(&FEXCore::HLE::x32::Set_thread_area),   1},
    {SYSCALL_EXIT_GROUP,               cvt(&FEXCore::HLE::Exit_group),             1},
    {SYSCALL_ARCH_PRCTL,               cvt(&FEXCore::HLE::Arch_Prctl),             2},
    {SYSCALL_OPENAT,                   cvt(&FEXCore::HLE::Openat),                 4},
    #endif
  };

  // Set all the new definitions
  for (auto &Syscall : Syscalls) {
    auto &Def = Definitions.at(std::get<0>(Syscall));
    LogMan::Throw::A(Def.Ptr == cvt(&Unimplemented), "Oops overwriting sysall problem");
    Def.Ptr = std::get<1>(Syscall);
    Def.NumArgs = std::get<2>(Syscall);
  }
}

uint64_t x32SyscallHandler::HandleSyscall(FEXCore::Core::InternalThreadState *Thread, FEXCore::HLE::SyscallArguments *Args) {
  auto &Def = Definitions[Args->Argument[0]];
  //LogMan::Msg::D(">>>>>Syscall: %d", Args->Argument[0]);
  switch (Def.NumArgs) {
  case 0: return std::invoke(Def.Ptr0, Thread);
  case 1: return std::invoke(Def.Ptr1, Thread, Args->Argument[1]);
  case 2: return std::invoke(Def.Ptr2, Thread, Args->Argument[1], Args->Argument[2]);
  case 3: return std::invoke(Def.Ptr3, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3]);
  case 4: return std::invoke(Def.Ptr4, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4]);
  case 5: return std::invoke(Def.Ptr5, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5]);
  case 6: return std::invoke(Def.Ptr6, Thread, Args->Argument[1], Args->Argument[2], Args->Argument[3], Args->Argument[4], Args->Argument[5], Args->Argument[6]);
  default: break;
  }

  LogMan::Msg::A("Unhandled syscall: %d", Args->Argument[0]);
  return -1;
}

FEXCore::SyscallHandler *CreateHandler(FEXCore::Context::Context *ctx) {
  return new x32SyscallHandler(ctx);
}

}
