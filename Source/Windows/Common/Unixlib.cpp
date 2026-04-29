// SPDX-License-Identifier: MIT
#include <windef.h>
#include <winternl.h>
#include <libloaderapi.h>
#include <wine/unixlib.h>

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace FEX::Windows::Unixlib {

static unixlib_handle_t UnixlibHandle;

#ifdef ARCHITECTURE_arm64ec
// On ARM64EC, indirect calls go through __os_arm64x_dispatch_icall which invokes
// FEX's custom call checker. Use a naked trampoline to bypass the dispatch mechanism
// and call the unix dispatcher directly via a register branch.
static CallDispatcher DispatcherDirect;

static NTSTATUS __attribute__((naked)) TrampolineCall(unixlib_handle_t, unsigned int, void*) {
  asm("adrp x16, %[disp]\n\t"
      "ldr x16, [x16, #:lo12:%[disp]]\n\t"
      "br x16" ::[disp] "S"(&DispatcherDirect));
}
#endif

void Init(HMODULE NtDll) {
  auto* DispatcherPtr = reinterpret_cast<CallDispatcher*>(GetProcAddress(NtDll, "__wine_unix_call_dispatcher"));
  if (!DispatcherPtr) {
    return;
  }

#ifdef ARCHITECTURE_arm64ec
  DispatcherDirect = *DispatcherPtr;
  Dispatcher = TrampolineCall;
#else
  Dispatcher = *DispatcherPtr;
#endif

  if (NtQueryVirtualMemory(NtCurrentProcess(), &__ImageBase, MemoryWineUnixFuncs, &UnixlibHandle, sizeof(UnixlibHandle), nullptr)) {
    Dispatcher = nullptr;
    return;
  }
  Handle = &UnixlibHandle;
}
} // namespace FEX::Windows::Unixlib
