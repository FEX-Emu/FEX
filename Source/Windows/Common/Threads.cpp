// SPDX-License-Identifier: MIT
#include "Threads.h"
#include "CRT/CRT.h"

#include <FEXCore/Utils/LogManager.h>
#include <FEXCore/Utils/Threads.h>
#include <FEXCore/fextl/memory.h>

#include <winternl.h>
#include <windows.h>

namespace FEX::Windows {
namespace WinThreadImpl {
  class Thread final : public FEXCore::Threads::Thread {
  public:
    Thread(FEXCore::Threads::ThreadFunc Func, void* Arg, FEXCore::Threads::Flags Flags)
      : UserFunc {Func}
      , UserArg {Arg}
      , Flags {Flags} {
      // hide everything from guest, don't initialize anything, we'll do that manually in RunThread()
      ULONG CreateFlags = THREAD_CREATE_FLAGS_SKIP_THREAD_ATTACH | THREAD_CREATE_FLAGS_SKIP_LOADER_INIT | THREAD_CREATE_FLAGS_BYPASS_PROCESS_FREEZE;
      if (Flags.Internal) {
        CreateFlags |= THREAD_CREATE_FLAGS_HIDE_FROM_DEBUGGER;
      }
      NTSTATUS Status = NtCreateThreadEx(&Handle, THREAD_ALL_ACCESS, nullptr, GetCurrentProcess(),
                                         reinterpret_cast<PRTL_THREAD_START_ROUTINE>(&Thread::RunThread), this, CreateFlags, 0, 0, 0, nullptr);
      if (Status < 0) {
        LogMan::Msg::EFmt("NtCreateThreadEx failed: 0x{:x}", static_cast<uint32_t>(Status));
        Handle = nullptr;
      }
    }

    bool joinable() override {
      return Handle != nullptr;
    }

    bool join(void** ret) override {
      if (!Handle) {
        return false;
      }
      NtWaitForSingleObject(Handle, FALSE, nullptr);
      if (ret) {
        *ret = ReturnValue;
      }
      return true;
    }

    bool detach() override {
      // almost no-op, it's already detached, just deref handle
      if (Handle) {
        NtClose(Handle);
        Handle = nullptr;
      }
      return true;
    }

    bool IsSelf() override {
      return TID == GetCurrentThreadId();
    }

    ~Thread() override {
      if (Handle) {
        NtClose(Handle);
      }
    }

    FEXCore::Threads::Flags GetFlags() const {
      return Flags;
    }

  private:
    static void RunThread(Thread* This) {
      This->TID = GetCurrentThreadId();
      if (This->GetFlags().LowPriority) {
        LONG Priority = THREAD_BASE_PRIORITY_IDLE;
        NtSetInformationThread(NtCurrentThread(), ThreadBasePriority, &Priority, sizeof(Priority));
      }
      // do initialization we skipped earlier here around the user entrypoint
      FEX::Windows::InitCRTThread();
      This->ReturnValue = This->UserFunc(This->UserArg);
      FEX::Windows::DeinitCRTThread();
      NtTerminateThread(GetCurrentThread(), 0);
    }

    FEXCore::Threads::ThreadFunc UserFunc;
    void* UserArg;
    FEXCore::Threads::Flags Flags {};
    HANDLE Handle {};
    DWORD TID {};
    void* ReturnValue {};
  };

  fextl::unique_ptr<FEXCore::Threads::Thread> CreateThread(FEXCore::Threads::ThreadFunc Func, void* Arg, FEXCore::Threads::Flags Flags) {
    return fextl::make_unique<Thread>(Func, Arg, Flags);
  }

  void CleanupAfterFork() {}
} // namespace WinThreadImpl

void SetupThreadHandlers() {
  FEXCore::Threads::Pointers Ptrs = {
    .CreateThread = WinThreadImpl::CreateThread,
    .CleanupAfterFork = WinThreadImpl::CleanupAfterFork,
  };
  FEXCore::Threads::Thread::SetInternalPointers(Ptrs);
}
} // namespace FEX::Windows
