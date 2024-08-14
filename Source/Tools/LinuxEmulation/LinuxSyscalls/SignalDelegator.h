// SPDX-License-Identifier: MIT
/*
$info$
tags: LinuxSyscalls|common
$end_info$
*/


#pragma once

#include "LinuxSyscalls/Types.h"
#include "ArchHelpers/MContext.h"
#include "VDSO_Emulation.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/fextl/memory.h>

#include <array>
#include <atomic>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <mutex>

#include <FEXCore/Core/SignalDelegator.h>
#include <FEXCore/Utils/ArchHelpers/Arm64.h>
#include <FEXCore/Utils/Telemetry.h>

namespace FEXCore {
namespace Context {
  class Context;
}
namespace Core {
  struct InternalThreadState;
}
} // namespace FEXCore

namespace FEX::HLE {
using HostSignalDelegatorFunction = std::function<bool(FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext)>;
using HostSignalDelegatorFunctionForGuest =
  std::function<bool(FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext, GuestSigAction* GuestAction, stack_t* GuestStack)>;

class SignalDelegator final : public FEXCore::SignalDelegator, public FEXCore::Allocator::FEXAllocOperators {
public:
  constexpr static size_t MAX_SIGNALS {64};

  // Use the last signal just so we are less likely to ever conflict with something that the guest application is using
  // 64 is used internally by Valgrind
  constexpr static size_t SIGNAL_FOR_PAUSE {63};

  // Returns true if the host handled the signal
  // Arguments are the same as sigaction handler
  SignalDelegator(FEXCore::Context::Context* _CTX, const std::string_view ApplicationName);
  ~SignalDelegator() override;

  // Called from the signal trampoline function.
  void HandleSignal(int Signal, void* Info, void* UContext);

  void RegisterTLSState(FEX::HLE::ThreadStateObject* Thread);
  void UninstallTLSState(FEX::HLE::ThreadStateObject* Thread);

  /**
   * @brief Registers a signal handler for the host to handle a signal
   *
   * It's a process level signal handler so one must be careful
   */
  void RegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required);

  /**
   * @brief Registers a signal handler for the host to handle a signal specifically for guest handling
   *
   * It's a process level signal handler so one must be careful
   */
  void RegisterHostSignalHandlerForGuest(int Signal, HostSignalDelegatorFunctionForGuest Func);
  void RegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required);

  /**
   * @name These functions are all for Linux signal emulation
   * @{ */
  /**
   * @brief Allows the guest to register a signal handler that is run after the host attempts to resolve the handler first
   */
  uint64_t RegisterGuestSignalHandler(int Signal, const GuestSigAction* Action, struct GuestSigAction* OldAction);

  uint64_t RegisterGuestSigAltStack(const stack_t* ss, stack_t* old_ss);

  uint64_t GuestSigProcMask(int how, const uint64_t* set, uint64_t* oldset);
  uint64_t GuestSigPending(uint64_t* set, size_t sigsetsize);
  uint64_t GuestSigSuspend(uint64_t* set, size_t sigsetsize);
  uint64_t GuestSigTimedWait(uint64_t* set, siginfo_t* info, const struct timespec* timeout, size_t sigsetsize);
  uint64_t GuestSignalFD(int fd, const uint64_t* set, size_t sigsetsize, int flags);
  /**  @} */

  /**
   * @brief Check to ensure the XID handler is still set to the FEX handler
   *
   * On a new thread GLIBC will set the XID handler underneath us.
   * After the first thread is created check this.
   */
  void CheckXIDHandler();

  void UninstallHostHandler(int Signal);
  FEXCore::Context::Context* CTX;

  void SetVDSOSigReturn() {
    // Get symbols from VDSO.
    VDSOPointers = FEX::VDSO::GetVDSOSymbols();

    // Update VDSO to generated code.
    // TODO: Have the frontend generate the x86 sigreturn pointers.
    CTX->GetVDSOSigReturn(&VDSOPointers);
  }

  [[noreturn]]
  void HandleSignalHandlerReturn(bool RT) {
    using SignalHandlerReturnFunc = void (*)();

    SignalHandlerReturnFunc SignalHandlerReturn {};
    if (RT) {
      SignalHandlerReturn = reinterpret_cast<SignalHandlerReturnFunc>(Config.SignalHandlerReturnAddressRT);
    } else {
      SignalHandlerReturn = reinterpret_cast<SignalHandlerReturnFunc>(Config.SignalHandlerReturnAddress);
    }

    SignalHandlerReturn();
    FEX_UNREACHABLE;
  }

  void SignalThread(FEXCore::Core::InternalThreadState* Thread, FEXCore::Core::SignalEvent Event) override;

  FEXCore::ArchHelpers::Arm64::UnalignedHandlerType GetUnalignedHandlerType() const {
    return UnalignedHandlerType;
  }

  void SaveTelemetry();
private:
  FEX::HLE::ThreadStateObject* GetTLSThread();

  // Called from the thunk handler to handle the signal
  void HandleGuestSignal(FEXCore::Core::InternalThreadState* Thread, int Signal, void* Info, void* UContext);

  /**
   * @brief Registers a signal handler for the host to handle a signal
   *
   * It's a process level signal handler so one must be careful
   */
  void FrontendRegisterHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required);
  void FrontendRegisterFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required);

  void SetHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
    HostHandlers[Signal].Handlers.push_back(std::move(Func));
  }
  void SetFrontendHostSignalHandler(int Signal, HostSignalDelegatorFunction Func, bool Required) {
    HostHandlers[Signal].FrontendHandler = std::move(Func);
  }

  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);
  FEX_CONFIG_OPT(Coredump, COREDUMP);
  FEX_CONFIG_OPT(Core, CORE);
  fextl::string const ApplicationName;
  FEXCORE_TELEMETRY_INIT(CrashMask, TYPE_CRASH_MASK);
  FEXCORE_TELEMETRY_INIT(UnhandledNonCanonical, TYPE_UNHANDLED_NONCANONICAL_ADDRESS);
  FEX_CONFIG_OPT(ParanoidTSO, PARANOIDTSO);
  FEX_CONFIG_OPT(HalfBarrierTSOEnabled, HALFBARRIERTSOENABLED);

  FEXCore::ArchHelpers::Arm64::UnalignedHandlerType UnalignedHandlerType {FEXCore::ArchHelpers::Arm64::UnalignedHandlerType::HalfBarrier};

  enum DefaultBehaviour {
    DEFAULT_TERM,
    // Core dump based signals are supposed to have a coredump appear
    // For FEX's behaviour we don't really care right now
    DEFAULT_COREDUMP = DEFAULT_TERM,
    DEFAULT_IGNORE,
  };

  struct kernel_sigaction {
    union {
      void (*handler)(int);
      void (*sigaction)(int, siginfo_t*, void*);
    };

    uint64_t sa_flags;

    void (*restorer)();
    uint64_t sa_mask;
  };

  struct SignalHandler {
    std::atomic<bool> Installed {};
    std::atomic<bool> Required {};
    kernel_sigaction HostAction {};
    kernel_sigaction OldAction {};
    FEX::HLE::HostSignalDelegatorFunctionForGuest GuestHandler {};
    GuestSigAction GuestAction {};
    DefaultBehaviour DefaultBehaviour {DEFAULT_TERM};

    // Callbacks
    fextl::vector<HostSignalDelegatorFunction> Handlers {};
    HostSignalDelegatorFunction FrontendHandler {};
  };

  std::array<SignalHandler, MAX_SIGNALS + 1> HostHandlers {};
  bool InstallHostThunk(int Signal);
  bool UpdateHostThunk(int Signal);

  FEXCore::Context::VDSOSigReturn VDSOPointers {};

  bool IsAddressInDispatcher(uint64_t Address) const {
    return Address >= Config.DispatcherBegin && Address < Config.DispatcherEnd;
  }

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
    uint32_t pretcode;                          ///< sigreturn return branch point.
    int32_t Signal;                             ///< The signal hit.
    FEXCore::x86::sigcontext sc;                ///< The signal context.
    FEXCore::x86::_libc_fpstate fpstate_unused; ///< Unused fpstate. Retained for backwards compatibility.
    uint32_t extramask[1];                      ///< Upper 32-bits of the signal mask. Lower 32-bits is in the sigcontext.
    char retcode[8];                            ///< Unused but needs to be filled. GDB seemingly uses as a debug marker.
    ///< FP state now follows after this.
  };

  // The 32-bit realtime signal frame.
  // This frame type is used when the guest signal is used with the `SA_SIGINFO` flag.
  struct RTSigFrame_i32 {
    uint32_t pretcode; ///< sigreturn return branch point.
    int32_t Signal;    ///< The signal hit.
    uint32_t pinfo;    ///< Pointer to siginfo_t
    uint32_t puc;      ///< Pointer to ucontext_t
    FEXCore::x86::siginfo_t info;
    FEXCore::x86::ucontext_t uc;
    char retcode[8]; ///< Unused but needs to be filled. GDB seemingly uses as a debug marker.
    ///< FP state now follows after this.
  };

  void SpillSRA(FEXCore::Core::InternalThreadState* Thread, void* ucontext, uint32_t IgnoreMask);

  void RestoreFrame_x64(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* Context,
                        FEXCore::Core::CpuStateFrame* Frame, void* ucontext);
  void RestoreFrame_ia32(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* Context,
                         FEXCore::Core::CpuStateFrame* Frame, void* ucontext);
  void RestoreRTFrame_ia32(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* Context,
                           FEXCore::Core::CpuStateFrame* Frame, void* ucontext);

  ///< Setup the signal frame for x64.
  uint64_t SetupFrame_x64(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* ContextBackup,
                          FEXCore::Core::CpuStateFrame* Frame, int Signal, siginfo_t* HostSigInfo, void* ucontext,
                          GuestSigAction* GuestAction, stack_t* GuestStack, uint64_t NewGuestSP, const uint32_t eflags);

  ///< Setup the signal frame for a 32-bit signal without SA_SIGINFO.
  uint64_t SetupFrame_ia32(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* ContextBackup,
                           FEXCore::Core::CpuStateFrame* Frame, int Signal, siginfo_t* HostSigInfo, void* ucontext,
                           GuestSigAction* GuestAction, stack_t* GuestStack, uint64_t NewGuestSP, const uint32_t eflags);

  ///< Setup the signal frame for a 32-bit signal with SA_SIGINFO.
  uint64_t SetupRTFrame_ia32(FEXCore::Core::InternalThreadState* Thread, ArchHelpers::Context::ContextBackup* ContextBackup,
                             FEXCore::Core::CpuStateFrame* Frame, int Signal, siginfo_t* HostSigInfo, void* ucontext,
                             GuestSigAction* GuestAction, stack_t* GuestStack, uint64_t NewGuestSP, const uint32_t eflags);

  enum class RestoreType {
    TYPE_REALTIME,    ///< Signal restore type is from a `realtime` signal.
    TYPE_NONREALTIME, ///< Signal restore type is from a `non-realtime` signal.
    TYPE_PAUSE,       ///< Signal restore type is from a GDB pause event.
  };
  ArchHelpers::Context::ContextBackup* StoreThreadState(FEXCore::Core::InternalThreadState* Thread, int Signal, void* ucontext);
  void RestoreThreadState(FEXCore::Core::InternalThreadState* Thread, void* ucontext, RestoreType Type);
  bool HandleDispatcherGuestSignal(FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext,
                                   GuestSigAction* GuestAction, stack_t* GuestStack);
  bool HandleSignalPause(FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext);
  bool HandleSIGILL(FEXCore::Core::InternalThreadState* Thread, int Signal, void* info, void* ucontext);

  std::mutex HostDelegatorMutex;
  std::mutex GuestDelegatorMutex;
};

fextl::unique_ptr<FEX::HLE::SignalDelegator> CreateSignalDelegator(FEXCore::Context::Context* CTX, const std::string_view ApplicationName);
} // namespace FEX::HLE
