// SPDX-License-Identifier: MIT
#include <FEXCore/Utils/LogManager.h>

#include <cstdint>
#include <windef.h>
#define __WINESRC__
#include <winternl.h>
#include <libloaderapi.h>
#include "FEXUnixLib.h"
#include "../UnixLib/FEXUnixLib.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace FEX::Windows::UnixLib {
unixlib_handle_t UnixLibHandle {};

decltype(__wine_unix_call_dispatcher) UnixCallDispatcher {};

#ifdef ARCHITECTURE_arm64ec
// On ARM64EC, indirect calls go through __os_arm64x_dispatch_icall which invokes
// FEX's custom call checker. Use a naked trampoline to bypass the dispatch mechanism
// and call the unix dispatcher directly via a register branch.
static decltype(__wine_unix_call_dispatcher) UnixCallDispatcherDirect {};

static NTSTATUS __attribute__((naked)) TrampolineCall(unixlib_handle_t, unsigned int, void*) {
  asm(R"(
  adrp x16, %[Displace];
  ldr x16, [x16, #:lo12:%[Displace]]
  br x16
  )" ::[Displace] "S"(&UnixCallDispatcherDirect)
      : "memory");
}
#endif

bool Init(HMODULE NtDll) {
  const auto Sym = GetProcAddress(NtDll, "__wine_unix_call_dispatcher");

  if (!Sym) {
    return false;
  }

  auto TryNewWineMethod = []() {
#ifdef ARCHITECTURE_arm64ec
    constexpr auto Name = "libarm64ecfex";
#else
    constexpr auto Name = "libwow64fex";
#endif

    // Not supported in Proton at all, but supported in upstream WINE.
    uint64_t Result[2];
    if (NtQueryVirtualMemory(NtCurrentProcess(), Name, MemoryWineLoadUnixLibByName, Result, sizeof(Result), nullptr)) {
      return false;
    }

    // Result[0] = unixlib_module_t
    // Result[1] = unixlib_handle_t
    // Module is ignored as it's only used to unload.
    UnixLibHandle = Result[1];
    return true;
  };

  auto TryOldWineMethod = []() {
    // Supported in Proton 11 and Experimental (2026-06-26).
    return NtQueryVirtualMemory(NtCurrentProcess(), &__ImageBase, MemoryWineUnixFuncs, &UnixLibHandle, sizeof(UnixLibHandle), nullptr) == 0;
  };

  if (!TryNewWineMethod() && !TryOldWineMethod()) {
    return false;
  }

#ifdef ARCHITECTURE_arm64ec
  UnixCallDispatcherDirect = *reinterpret_cast<decltype(__wine_unix_call_dispatcher)*>(Sym);
  UnixCallDispatcher = TrampolineCall;
#else
  UnixCallDispatcher = *reinterpret_cast<decltype(__wine_unix_call_dispatcher)*>(Sym);
#endif

  // Give a log saying that the unix lib was loaded.
  LogMan::Msg::IFmt("FEX: Loaded FEXUnixLib");
  return true;
}

static bool UnixLibAvailable() {
  return UnixLibHandle != 0;
}

} // namespace FEX::Windows::UnixLib
