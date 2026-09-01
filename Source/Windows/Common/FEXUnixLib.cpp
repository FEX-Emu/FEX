// SPDX-License-Identifier: MIT
#include <FEXCore/fextl/fmt.h>
#include <FEXCore/Utils/LogManager.h>

#include <cstdint>
#include <windef.h>
#include <winbase.h>
#define __WINESRC__
#include <winternl.h>
#include <libloaderapi.h>
#include "FEXUnixLib.h"
#include "Priv.h"

#define PR_SET_VMA 0x53564d41
#define PR_SET_VMA_ANON_NAME 0

#define MADV_HUGEPAGE 14
#define MADV_NOHUGEPAGE 15

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace FEX::Windows::UnixLib {
static bool SupportsVirtualName {true};
unixlib_handle_t UnixLibHandle {};

decltype(__wine_unix_call_dispatcher) UnixCallDispatcher {};

using wine_server_handle_to_fd_t = NTSTATUS(CDECL*)(HANDLE, unsigned int, int*, unsigned int*);
static wine_server_handle_to_fd_t WineServerHandleToFd {};

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
  WineServerHandleToFd = reinterpret_cast<wine_server_handle_to_fd_t>(GetProcAddress(NtDll, "wine_server_handle_to_fd"));

  if (!Sym) {
    return false;
  }

  auto TryNewWineMethod = []() {
#ifdef ARCHITECTURE_arm64ec
    auto Name = InitUnicodeString(L"libarm64ecfex");
#else
    auto Name = InitUnicodeString(L"libwow64fex");
#endif

    // Not supported in Proton at all, but supported in upstream WINE.
    uint64_t Result[2];
    if (NtQueryVirtualMemory(NtCurrentProcess(), &Name, MemoryWineLoadUnixLibByName, Result, sizeof(Result), nullptr)) {
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

bool Available() {
  return UnixLibHandle != 0;
}

bool TryEnableHardwareTSO() {
  if (!Available()) {
    return false;
  }

  FEXUnixLib_SetHardwareTSOControlArgs Args {
    .Enable = true,
  };

  return Call(FEXUnixLibFunctions::SetHardwareTSOControl, &Args) == STATUS_SUCCESS;
}

bool SetKernelUnalignedAtomicControl(uint64_t Flags) {
  if (!Available()) {
    return false;
  }

  FEXUnixLib_SetKernelUnalignedAtomicControl Args {
    .Flags = Flags,
  };

  return Call(FEXUnixLibFunctions::SetKernelUnalignedAtomicControl, &Args) == STATUS_SUCCESS;
}

void VirtualTHPControl(const void* Ptr, size_t Size, FEXCore::Allocator::THPControl Control) {
  if (!Available()) {
    return;
  }

  FEXUnixLib_Madvise Args {
    .Addr = Ptr,
    .Size = Size,
    .Advise = Control == FEXCore::Allocator::THPControl::Disable ? MADV_NOHUGEPAGE : MADV_HUGEPAGE,
  };

  Call(FEXUnixLibFunctions::Madvise, &Args);
}

void VirtualName(const char* Name, const void* Ptr, size_t Size) {
  if (!SupportsVirtualName || !Available()) {
    return;
  }

  FEXUnixLib_SetVMAName Args {
    .Addr = Ptr,
    .Size = Size,
    .Name = Name,
  };

  if (Call(FEXUnixLibFunctions::SetVMAName, &Args) != STATUS_SUCCESS) {
    SupportsVirtualName = false;
  }
}

SHMSlotResult AllocateSHMSlots(void* SHMBase, uint32_t MapSize, uint32_t MaxSize) {
  if (!Available()) {
    return {};
  }

  FEXUnixLib_GetSHMStatsVMA Args {
    .SHMBase = SHMBase,
    .MapSize = MapSize,
    .MaxSize = MaxSize,
  };

  if (Call(FEXUnixLibFunctions::GetSHMStatsVMA, &Args) == STATUS_SUCCESS) {
    return {
      .SHMBase = Args.SHMBase,
      .MappedSize = Args.MapSize,
    };
  }

  return {};
}

void DeleteSHMStatsFile() {
  if (!Available()) {
    return;
  }

  Call(FEXUnixLibFunctions::DeleteSHMStatsFile, nullptr);
}

void* MapFile(HANDLE FileHandle, uint64_t MapSize) {
  if (Available() && WineServerHandleToFd) {
    FEXUnixLib_MapFile Args {
      .FD = -1,
      .MapSize = MapSize,
    };

    // this is a dup equivalent
    auto Result = WineServerHandleToFd(FileHandle, FILE_READ_DATA, &Args.FD, nullptr);
    if (Result != STATUS_SUCCESS || Args.FD == -1) {
      return nullptr;
    }

    if (Call(FEXUnixLibFunctions::MapFile, &Args) == STATUS_SUCCESS) {
      return Args.Result;
    }
  }

  return nullptr;
}

} // namespace FEX::Windows::UnixLib
