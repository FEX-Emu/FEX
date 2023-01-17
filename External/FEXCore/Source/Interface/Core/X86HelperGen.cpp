/*
$info$
tags: glue|x86-guest-code
desc: Guest-side assembly helpers used by the backends
$end_info$
*/

#include "Interface/Core/X86HelperGen.h"

#include <FEXCore/Config/Config.h>
#include <FEXCore/Utils/Allocator.h>
#include <FEXHeaderUtils/Syscalls.h>

#include <cstdint>
#include <cstring>
#include <vector>
#include <sys/mman.h>

namespace FEXCore {
constexpr size_t CODE_SIZE = 0x1000;

X86GeneratedCode::X86GeneratedCode() {
  // Allocate a page for our emulated guest
  CodePtr = AllocateGuestCodeSpace(CODE_SIZE);

  SignalReturn = reinterpret_cast<uint64_t>(CodePtr);
  SignalReturnRT = reinterpret_cast<uint64_t>(CodePtr) + 7;
  CallbackReturn = reinterpret_cast<uint64_t>(CodePtr) + 14;

  const std::vector<uint8_t> SignalReturnCode = {
    // sigreturn
    0xb8, 0x77, 0x00, 0x00, 0x00, // mov eax, SYS_sigreturn
    0xcd, 0x80, // int 0x80

    // rt_sigreturn
    0xb8, 0xad, 0x00, 0x00, 0x00, // mov eax, SYS_rt_sigreturn
    0xcd, 0x80, // int 0x80

    0x0F, 0x37, // CALLBACKRET FEX Instruction
  };

  memcpy(CodePtr, &SignalReturnCode.at(0), SignalReturnCode.size());
}

X86GeneratedCode::~X86GeneratedCode() {
  FEXCore::Allocator::munmap(CodePtr, CODE_SIZE);
}

void* X86GeneratedCode::AllocateGuestCodeSpace(size_t Size) {
  FEX_CONFIG_OPT(Is64BitMode, IS64BIT_MODE);

  if (Is64BitMode()) {
    // 64bit mode can have its sigret handler anywhere
    return FEXCore::Allocator::mmap(nullptr, Size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  }

  // First 64bit page
  constexpr uintptr_t LOCATION_MAX = 0x1'0000'0000;

  // 32bit mode
  // We need to have the sigret handler in the lower 32bits of memory space
  // Scan top down and try to allocate a location
  for (size_t Location = 0xFFFF'E000; Location != 0x0; Location -= 0x1000) {
    void *Ptr = ::mmap(reinterpret_cast<void*>(Location), Size, PROT_READ | PROT_WRITE, MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (Ptr != MAP_FAILED &&
        reinterpret_cast<uintptr_t>(Ptr) >= LOCATION_MAX) {
      // Failed to map in the lower 32bits
      // Try again
      // Can happen in the case that host kernel ignores MAP_FIXED_NOREPLACE
      ::munmap(Ptr, Size);
      continue;
    }

    if (Ptr != MAP_FAILED) {
      return Ptr;
    }
  }

  // Can't do anything about this
  // Here's hoping the application doesn't use signals
  return MAP_FAILED;
}

}

