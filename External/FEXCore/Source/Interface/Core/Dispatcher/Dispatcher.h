#pragma once

#include <FEXCore/Core/CPUBackend.h>
#include "Interface/Core/ArchHelpers/MContext.h"

#include <cstdint>
#include <signal.h>
#include <stddef.h>
#include <stack>
#include <tuple>
#include <vector>

namespace FEXCore {
struct GuestSigAction;
}

namespace FEXCore::Core {
struct CpuStateFrame;
struct InternalThreadState;
}

namespace FEXCore::Context {
struct Context;
}

namespace FEXCore::CPU {

struct DispatcherConfig {
  bool StaticRegisterAllocation = false;
};

class Dispatcher {
public:
  virtual ~Dispatcher() = default;

  /**
   * @name Dispatch Helper functions
   * @{ */
  uint64_t ThreadStopHandlerAddress{};
  uint64_t ThreadStopHandlerAddressSpillSRA{};
  uint64_t AbsoluteLoopTopAddress{};
  uint64_t AbsoluteLoopTopAddressFillSRA{};
  uint64_t ThreadPauseHandlerAddress{};
  uint64_t ThreadPauseHandlerAddressSpillSRA{};
  uint64_t ExitFunctionLinkerAddress{};
  uint64_t SignalHandlerReturnAddress{};
  uint64_t SignalHandlerReturnAddressRT{};
  uint64_t GuestSignal_SIGILL{};
  uint64_t GuestSignal_SIGTRAP{};
  uint64_t GuestSignal_SIGSEGV{};
  uint64_t IntCallbackReturnAddress{};

  uint64_t PauseReturnInstruction{};

  /**  @} */

  uint64_t Start{};
  uint64_t End{};

  bool HandleGuestSignal(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext, GuestSigAction *GuestAction, stack_t *GuestStack);
  bool HandleSIGILL(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext);
  bool HandleSignalPause(FEXCore::Core::InternalThreadState *Thread, int Signal, void *info, void *ucontext);

  bool IsAddressInDispatcher(uint64_t Address) const {
    return Address >= Start && Address < End;
  }

  virtual void InitThreadPointers(FEXCore::Core::InternalThreadState *Thread) = 0;

  // These are across all arches for now
  static constexpr size_t MaxGDBPauseCheckSize = 128;
  static constexpr size_t MaxInterpreterTrampolineSize = 128;

  virtual size_t GenerateGDBPauseCheck(uint8_t *CodeBuffer, uint64_t GuestRIP) = 0;
  virtual size_t GenerateInterpreterTrampoline(uint8_t *CodeBuffer) = 0;

  static std::unique_ptr<Dispatcher> CreateX86(FEXCore::Context::Context *CTX, const DispatcherConfig &Config);
  static std::unique_ptr<Dispatcher> CreateArm64(FEXCore::Context::Context *CTX, const DispatcherConfig &Config);

  virtual void ExecuteDispatch(FEXCore::Core::CpuStateFrame *Frame) {
    DispatchPtr(Frame);
  }

  virtual void ExecuteJITCallback(FEXCore::Core::CpuStateFrame *Frame, uint64_t RIP) {
    CallbackPtr(Frame, RIP);
  }

protected:
  Dispatcher(FEXCore::Context::Context *ctx, const DispatcherConfig &Config)
    : CTX {ctx}
    , config {Config}
    {}

  void RestoreFrame_x64(ArchHelpers::Context::ContextBackup* Context, FEXCore::Core::CpuStateFrame *Frame, void *ucontext);
  void RestoreFrame_ia32(ArchHelpers::Context::ContextBackup* Context, FEXCore::Core::CpuStateFrame *Frame, void *ucontext);
  void RestoreRTFrame_ia32(ArchHelpers::Context::ContextBackup* Context, FEXCore::Core::CpuStateFrame *Frame, void *ucontext);

  ///< Setup the signal frame for x64.
  uint64_t SetupFrame_x64(FEXCore::Core::InternalThreadState *Thread, ArchHelpers::Context::ContextBackup* ContextBackup, FEXCore::Core::CpuStateFrame *Frame,
    int Signal, siginfo_t *HostSigInfo, void *ucontext,
    GuestSigAction *GuestAction, stack_t *GuestStack,
    uint64_t NewGuestSP, const uint32_t eflags);

  ///< Setup the signal frame for a 32-bit signal without SA_SIGINFO.
  uint64_t SetupFrame_ia32(ArchHelpers::Context::ContextBackup* ContextBackup, FEXCore::Core::CpuStateFrame *Frame,
    int Signal, siginfo_t *HostSigInfo, void *ucontext,
    GuestSigAction *GuestAction, stack_t *GuestStack,
    uint64_t NewGuestSP, const uint32_t eflags);

  ///< Setup the signal frame for a 32-bit signal with SA_SIGINFO.
  uint64_t SetupRTFrame_ia32(ArchHelpers::Context::ContextBackup* ContextBackup, FEXCore::Core::CpuStateFrame *Frame,
    int Signal, siginfo_t *HostSigInfo, void *ucontext,
    GuestSigAction *GuestAction, stack_t *GuestStack,
    uint64_t NewGuestSP, const uint32_t eflags);

  ArchHelpers::Context::ContextBackup* StoreThreadState(FEXCore::Core::InternalThreadState *Thread, int Signal, void *ucontext);
  enum class RestoreType {
    TYPE_REALTIME, ///< Signal restore type is from a `realtime` signal.
    TYPE_NONREALTIME, ///< Signal restore type is from a `non-realtime` signal.
    TYPE_PAUSE, ///< Signal restore type is from a GDB pause event.
  };

  /*
   * Signal frames on 32-bit architecture needs to match exactly how the kernel generates the frame.
   * This is because large parts of the signal frame definition is part of the UAPI.
   * This means that when FEX sets up the signal frame, it needs to match the UAPI stack setup.
   *
   * The two signal stack frame types below describe the two different 32-bit frame types.
   */

  // The 32-bit non-realtime signal frame.
  // This frame type is used when the guest signal is used without the `SA_SIGINFO` flag.
  struct SigFrame_i32 {
    uint32_t pretcode; ///< sigreturn return branch point.
    int32_t Signal; ///< The signal hit.
    FEXCore::x86::sigcontext sc; ///< The signal context.
    x86::_libc_fpstate fpstate_unused; ///< Unused fpstate. Retained for backwards compatibility.
    uint32_t extramask[1]; ///< Upper 32-bits of the signal mask. Lower 32-bits is in the sigcontext.
    char retcode[8]; ///< Unused but needs to be filled. GDB seemingly uses as a debug marker.
    ///< FP state now follows after this.
  };

  // The 32-bit realtime signal frame.
  // This frame type is used when the guest signal is used with the `SA_SIGINFO` flag.
  struct RTSigFrame_i32 {
    uint32_t pretcode; ///< sigreturn return branch point.
    int32_t Signal; ///< The signal hit.
    uint32_t pinfo; ///< Pointer to siginfo_t
    uint32_t puc; ///< Pointer to ucontext_t
    FEXCore::x86::siginfo_t info;
    FEXCore::x86::ucontext_t uc;
    char retcode[8]; ///< Unused but needs to be filled. GDB seemingly uses as a debug marker.
    ///< FP state now follows after this.
  };

  void RestoreThreadState(FEXCore::Core::InternalThreadState *Thread, void *ucontext, RestoreType Type);
  std::stack<uint64_t, std::vector<uint64_t>> SignalFrames;

  virtual void SpillSRA(FEXCore::Core::InternalThreadState *Thread, void *ucontext, uint32_t IgnoreMask) {}

  FEXCore::Context::Context *CTX;
  DispatcherConfig config;

  static void SleepThread(FEXCore::Context::Context *ctx, FEXCore::Core::CpuStateFrame *Frame);

  static uint64_t GetCompileBlockPtr();

  using AsmDispatch = void(*)(FEXCore::Core::CpuStateFrame *Frame);
  using JITCallback = void(*)(FEXCore::Core::CpuStateFrame *Frame, uint64_t RIP);

  AsmDispatch DispatchPtr;
  JITCallback CallbackPtr;
};

}
