// SPDX-License-Identifier: MIT
#include <FEXCore/Utils/AllocatorHooks.h>
#include <FEXCore/Utils/TypeDefines.h>

#include <array>
#include <chrono>
#include <libloaderapi.h>
#include <sysinfoapi.h>
#include <synchapi.h>
#include <windef.h>
#include <winternl.h>
#include <winnt.h>
#include <wine/debug.h>

namespace FEX::Windows::Allocator {
#define PR_SET_VMA 0x53564d41
#define PR_SET_VMA_ANON_NAME 0

#define MADV_HUGEPAGE 14
#define MADV_NOHUGEPAGE 15

namespace Trampoline {
  struct madvise_data {
    void* addr;
    size_t size;
    int advise;
  };

  struct prctl_data {
    int op;
    uint64_t attr;
    void* addr;
    size_t size;
    const char* name;
    uint64_t ret;
  };

  __attribute__((naked)) uint64_t wine_prctl(prctl_data* d) {
    asm volatile(
      R"(
  .globl wine_prctl_begin
  wine_prctl_begin:
    mov x19, x0;
    mov x8, 167; // prctl
    ldr x0, [x19]; // op
    ldp x1, x2, [x19, %[attr_offset]]; // {attr, addr}
    ldp x3, x4, [x19, %[size_offset]]; // {size, name}
    svc #0;
    str x0, [x19, %[ret_offset]];
    // Tell wine it was all groovy.
    mov x0, 0;
    ret;

  .globl wine_prctl_end
  wine_prctl_end:
  )"
      :
      : [attr_offset] "i"(offsetof(prctl_data, attr)), [size_offset] "i"(offsetof(prctl_data, size)), [ret_offset] "i"(offsetof(prctl_data, ret))
      : "memory");
  };

  __attribute__((naked)) uint64_t wine_madvise(madvise_data* d) {
    asm volatile(R"(
  .globl wine_madvise_begin
  wine_madvise_begin:
    mov x8, 233; // madvise
    ldr x2, [x0, %[advise_offset]]; // advise
    ldp x0, x1, [x0]; // {addr, size}
    svc #0;
    // Tell wine it was all groovy.
    mov x0, 0;
    ret;

  .globl wine_madvise_end
  wine_madvise_end:
  )" ::[advise_offset] "i"(offsetof(madvise_data, advise))
                 : "memory");
  }

  extern "C" uint64_t wine_madvise_begin;
  extern "C" uint64_t wine_madvise_end;

  void* const wine_madvise_begin_loc = &wine_madvise_begin;
  void* const wine_madvise_end_loc = &wine_madvise_end;

  extern "C" uint64_t wine_prctl_begin;
  extern "C" uint64_t wine_prctl_end;

  void* const wine_prctl_begin_loc = &wine_prctl_begin;
  void* const wine_prctl_end_loc = &wine_prctl_end;

  extern NTSTATUS(WINAPI* __wine_unix_call_dispatcher)(uint64_t, unsigned int, void*);
  decltype(__wine_unix_call_dispatcher) WineUnixCall;

  enum unix_function_indexes {
    INDEX_PRCTL = 0,
    INDEX_MADVISE = 1,
    INDEX_MAX,
  };
  static std::array<void*, INDEX_MAX> unix_functions {};

  static uint64_t wine_prctl_trampoline(int op, uint64_t attr, void* addr, size_t size, const char* name) {
    prctl_data d {
      .op = op,
      .attr = attr,
      .addr = addr,
      .size = size,
      .name = name,
    };
    WineUnixCall(reinterpret_cast<uint64_t>(unix_functions.data()), INDEX_PRCTL, &d);
    return d.ret;
  }

  static uint64_t wine_madvise_trampoline(void* addr, size_t size, int advice) {
    madvise_data d {
      .addr = addr,
      .size = size,
      .advise = advice,
    };
    WineUnixCall(reinterpret_cast<uint64_t>(unix_functions.data()), INDEX_MADVISE, &d);
    return 0;
  }

  void VirtualName(const char* Name, void* Ptr, size_t Size) {
    static bool Supports {true};
    if (Supports) {
      auto Result = wine_prctl_trampoline(PR_SET_VMA, PR_SET_VMA_ANON_NAME, Ptr, Size, Name);
      if (Result != 0) {
        // Disable any additional attempts.
        Supports = false;
      }
    }
  }

  void VirtualTHPControl(void* Ptr, size_t Size, FEXCore::Allocator::THPControl Control) {
    wine_madvise_trampoline(Ptr, Size, Control == FEXCore::Allocator::THPControl::RequestNoHugeTLB ? MADV_NOHUGEPAGE : MADV_HUGEPAGE);
  }
} // namespace Trampoline

namespace Illegal {
  __attribute__((naked)) uint64_t prctl(int op, uint64_t attr, void* addr, size_t size, const char* Name) {
    asm volatile(R"(
      mov x8, 167; // prctl
      svc #0;
      ret;
    )" ::
                   : "memory");
  }

  __attribute__((naked)) uint64_t madvise(void* addr, size_t size, int advice) {
    asm volatile(R"(
      mov x8, 233; // madvise
      svc #0;
      ret;
    )" ::
                   : "memory");
  }

  void VirtualName(const char* Name, void* Ptr, size_t Size) {
    static bool Supports {true};
    if (Supports) {
      auto Result = prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, Ptr, Size, Name);
      if (Result != 0) {
        // Disable any additional attempts.
        Supports = false;
      }
    }
  }

  void VirtualTHPControl(void* Ptr, size_t Size, FEXCore::Allocator::THPControl Control) {
    madvise(Ptr, Size, Control == FEXCore::Allocator::THPControl::RequestNoHugeTLB ? MADV_NOHUGEPAGE : MADV_HUGEPAGE);
  }
} // namespace Illegal

// NTSTATUS __wine_unix_call_dispatcher( unixlib_handle_t, unsigned int, void * );
// - unixlib_handle_t is just an array of functions
// - uint32_t is just an index in to that
// - void* is the user provided pointer, gets loaded in to x0 in for the unix_function called.
// - Return value - SUCCESS or other error.
void SetupHooks(HMODULE ntdll) {
  const auto Sym = GetProcAddress(ntdll, "__wine_unix_call_dispatcher");

  if (!Sym) {
    return;
  }

  FEXCore::Allocator::HookPtrs Ptrs {};

  // Wine wants us to use trampolines through unixlib.
  // This doesn't seem to work for some reason so skip it for now.
  if constexpr (false) {
    Trampoline::WineUnixCall = *reinterpret_cast<decltype(Trampoline::WineUnixCall)*>(Sym);

    // This code must be copied over to allocated memory from top down allocations apparently.
    auto Code = reinterpret_cast<uint8_t*>(
      ::VirtualAlloc(nullptr, FEXCore::Utils::FEX_PAGE_SIZE, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE));
    if (!Code) {
      return;
    }

    size_t CurrentOffset {};
    const size_t prctl_size =
      reinterpret_cast<uintptr_t>(Trampoline::wine_prctl_end_loc) - reinterpret_cast<uintptr_t>(Trampoline::wine_prctl_begin_loc);
    const size_t madvise_size =
      reinterpret_cast<uintptr_t>(Trampoline::wine_madvise_end_loc) - reinterpret_cast<uintptr_t>(Trampoline::wine_madvise_begin_loc);

    // Copy prctl.
    memcpy(Code + CurrentOffset, Trampoline::wine_prctl_begin_loc, prctl_size);
    Trampoline::unix_functions[Trampoline::INDEX_PRCTL] = Code + CurrentOffset;
    CurrentOffset += prctl_size;

    // Copy madvise.
    memcpy(Code + CurrentOffset, Trampoline::wine_madvise_begin_loc, madvise_size);
    Trampoline::unix_functions[Trampoline::INDEX_MADVISE] = Code + CurrentOffset;

    // Protect the page now.
    FEXCore::Allocator::VirtualProtect(Code, FEXCore::Utils::FEX_PAGE_SIZE,
                                       FEXCore::Allocator::ProtectOptions::Read | FEXCore::Allocator::ProtectOptions::Exec);

    Ptrs = {
      .VirtualName = Trampoline::VirtualName,
      .VirtualTHPControl = Trampoline::VirtualTHPControl,
    };
  } else {
    Ptrs = {
      .VirtualName = Illegal::VirtualName,
      .VirtualTHPControl = Illegal::VirtualTHPControl,
    };
  }

  SYSTEM_INFO system_info {};
  GetSystemInfo(&system_info);
  FEXCore::Allocator::SetupHooks(system_info.dwPageSize, Ptrs);
}
} // namespace FEX::Windows::Allocator
