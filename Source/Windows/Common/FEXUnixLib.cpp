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
static bool UsingNTQueryPath {};
static bool SupportsVirtualName {true};
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
// This code path will eventually crash once Wine and the kernel implements `userspace syscall dispatch`.
// FEX will need to switch over to using WINE's unixlib syscall approach then.
namespace Illegal {
  __attribute__((naked)) uint64_t prctl(int op, uint64_t attr, const void* addr, size_t size, const char* Name) {
    asm volatile(R"(
      mov x8, 167; // prctl
      svc #0;
      ret;
    )" ::
                   : "memory");
  }

  __attribute__((naked)) uint64_t madvise(const void* addr, size_t size, int advice) {
    asm volatile(R"(
      mov x8, 233; // madvise
      svc #0;
      ret;
    )" ::
                   : "memory");
  }

  __attribute__((naked)) uint64_t linux_getpid() {
    asm volatile(R"(
    mov x8, 172;
    svc #0;
    ret;
    )" ::
                   : "r0", "r8");
  }

  void VirtualName(const char* Name, const void* Ptr, size_t Size) {
    if (SupportsVirtualName) {
      auto Result = prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, Ptr, Size, Name);
      if (Result != 0) {
        // Disable any additional attempts.
        SupportsVirtualName = false;
      }
    }
  }

  void VirtualTHPControl(const void* Ptr, size_t Size, FEXCore::Allocator::THPControl Control) {
    madvise(Ptr, Size, Control == FEXCore::Allocator::THPControl::Disable ? MADV_NOHUGEPAGE : MADV_HUGEPAGE);
  }
} // namespace Illegal

bool Init(HMODULE NtDll) {
  const auto Sym = GetProcAddress(NtDll, "__wine_unix_call_dispatcher");

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

static bool UnixLibAvailable() {
  return UnixLibHandle != 0;
}

bool TryEnableHardwareTSO() {
  if (UnixLibAvailable()) {
    // UnixLib path.
    FEXUnixLib_SetHardwareTSOControlArgs Args {
      .Enable = true,
    };

    return Call(FEXUnixLibFunctions::SetHardwareTSOControl, &Args) == STATUS_SUCCESS;
  }

  // Legacy Proton path.
  BOOL Enable = TRUE;
  NTSTATUS Status = NtSetInformationProcess(NtCurrentProcess(), ProcessFexHardwareTso, &Enable, sizeof(Enable));
  return Status == STATUS_SUCCESS;
}

bool SetKernelUnalignedAtomicControl(uint64_t Flags) {
  if (UnixLibAvailable()) {
    // UnixLib path.
    FEXUnixLib_SetKernelUnalignedAtomicControl Args {
      .Flags = Flags,
    };

    return Call(FEXUnixLibFunctions::SetKernelUnalignedAtomicControl, &Args) == STATUS_SUCCESS;
  }

  // Legacy Proton path.
  return NtSetInformationProcess(NtCurrentProcess(), ProcessFexUnalignAtomic, &Flags, sizeof(Flags)) == STATUS_SUCCESS;
}

void VirtualTHPControl(const void* Ptr, size_t Size, FEXCore::Allocator::THPControl Control) {
  if (UnixLibAvailable()) {
    // UnixLib path.
    FEXUnixLib_Madvise Args {
      .Addr = Ptr,
      .Size = Size,
      .Advise = Control == FEXCore::Allocator::THPControl::Disable ? MADV_NOHUGEPAGE : MADV_HUGEPAGE,
    };

    Call(FEXUnixLibFunctions::Madvise, &Args);
    return;
  }

  // Legacy Proton path.
  Illegal::VirtualTHPControl(Ptr, Size, Control);
}

void VirtualName(const char* Name, const void* Ptr, size_t Size) {
  if (!SupportsVirtualName) {
    return;
  }

  if (UnixLibAvailable()) {
    // UnixLib path.
    FEXUnixLib_SetVMAName Args {
      .Addr = Ptr,
      .Size = Size,
      .Name = Name,
    };

    if (Call(FEXUnixLibFunctions::SetVMAName, &Args) != STATUS_SUCCESS) {
      SupportsVirtualName = false;
    }
    return;
  }

  // Legacy Proton path.
  Illegal::VirtualName(Name, Ptr, Size);
}

SHMSlotResult AllocateSHMSlots(void* SHMBase, uint32_t MapSize, uint32_t MaxSize) {
  if (UnixLibAvailable()) {
    // UnixLib path.
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

  // Legacy Proton path.
  // Magic WINE+FEX path.
  if (SHMBase == nullptr || UsingNTQueryPath) {
    MEMORY_FEX_STATS_SHM_INFORMATION Info {
      .shm_base = SHMBase,
      .map_size = MapSize,
      .max_size = MaxSize,
    };
    size_t Length {};
    auto Result = NtQueryVirtualMemory(NtCurrentProcess(), nullptr, MemoryFexStatsShm, &Info, sizeof(Info), &Length);
    if (!Result) {
      UsingNTQueryPath = true;
      return {
        .SHMBase = Info.shm_base,
        .MappedSize = MapSize,
      };
    }
  }

  // Opaque handle path, doesn't support resizing.
  auto handle = CreateFile(fextl::fmt::format("/dev/shm/fex-{}-stats", Illegal::linux_getpid()).c_str(), GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  // Create the section mapping for the file handle for the full size.
  HANDLE SectionMapping;
  LARGE_INTEGER SectionSize {{MaxSize}};
  auto Result = NtCreateSection(&SectionMapping, SECTION_EXTEND_SIZE | SECTION_MAP_READ | SECTION_MAP_WRITE, nullptr, &SectionSize,
                                PAGE_READWRITE, SEC_COMMIT, handle);
  if (Result != STATUS_SUCCESS) {
    CloseHandle(handle);
    return {};
  }

  // Section mapping is used from now on.
  CloseHandle(handle);

  // Now actually map the view of the section.
  void* Base = nullptr;
  size_t FullSize = MaxSize;
  Result = NtMapViewOfSection(SectionMapping, NtCurrentProcess(), &Base, 0, 0, nullptr, &FullSize, ViewUnmap, MEM_RESERVE | MEM_TOP_DOWN,
                              PAGE_READWRITE);
  if (Result != STATUS_SUCCESS) {
    CloseHandle(SectionMapping);
    return {};
  }

  return {.SHMBase = Base, .MappedSize = MaxSize};
}

void DeleteSHMStatsFile() {
  if (UnixLibAvailable()) {
    // UnixLib path.
    Call(FEXUnixLibFunctions::DeleteSHMStatsFile, nullptr);
    return;
  }

  // Legacy Proton path.
  DeleteFile(fextl::fmt::format("/dev/shm/fex-{}-stats", Illegal::linux_getpid()).c_str());
}

} // namespace FEX::Windows::UnixLib
