// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/Syscalls.h"

namespace FEX::HLE::FaultSafeMemcpy {
#ifdef _M_ARM_64
__attribute__((naked))
size_t CopyFromUser(void *Dest, const void* Src, size_t Size) {
  __asm volatile(R"(
  // Early exit if a memcpy of size zero.
  cbz x2, 2f;

  1:
  .globl CopyFromUser_FaultInst
  CopyFromUser_FaultInst:
    ldrb w3, [x1], 1; // <- This line can fault.
    strb w3, [x0], 1;
    sub x2, x2, 1;
    cbnz x2, 1b;
2:
    mov x0, 0;
    ret;
  )"
  ::: "memory");
}

__attribute__((naked))
size_t CopyToUser(void *Dest, const void* Src, size_t Size) {
  __asm volatile(R"(
  // Early exit if a memcpy of size zero.
  cbz x2, 2f;

  1:
    ldrb w3, [x1], 1;
  .globl CopyToUser_FaultInst
  CopyToUser_FaultInst:
    strb w3, [x0], 1; // <- This line can fault.
    sub x2, x2, 1;
    cbnz x2, 1b;
2:
    mov x0, 0;
    ret;
  )"
  ::: "memory");
}

extern "C" uint64_t CopyFromUser_FaultInst;
void * const CopyFromUser_FaultLocation = &CopyFromUser_FaultInst;

extern "C" uint64_t CopyToUser_FaultInst;
void * const CopyToUser_FaultLocation = &CopyToUser_FaultInst;

bool IsFaultLocation(uint64_t PC) {
  return reinterpret_cast<void*>(PC) == CopyFromUser_FaultLocation ||
    reinterpret_cast<void*>(PC) == CopyToUser_FaultLocation;
}

#else
size_t CopyFromUser(void *Dest, const void* Src, size_t Size) {
  memcpy(Dest, Src, Size);
  return Size;
}

size_t CopyToUser(void *Dest, const void* Src, size_t Size) {
  memcpy(Dest, Src, Size);
  return Size;
}

bool IsFaultLocation(uint64_t PC) {
  return false;
}
#endif
}
