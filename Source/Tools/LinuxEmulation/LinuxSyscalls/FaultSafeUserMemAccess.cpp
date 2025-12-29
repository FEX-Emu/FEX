// SPDX-License-Identifier: MIT
#include "LinuxSyscalls/Syscalls.h"

namespace FEX::HLE::FaultSafeUserMemAccess {
#ifdef ARCHITECTURE_arm64
__attribute__((naked)) size_t CopyFromUser(void* Dest, const void* Src, size_t Size) {
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
  )" ::
                   : "memory");
}

__attribute__((naked)) size_t CopyToUser(void* Dest, const void* Src, size_t Size) {
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
  )" ::
                   : "memory");
}

extern "C" uint64_t CopyFromUser_FaultInst;
void* const CopyFromUser_FaultLocation = &CopyFromUser_FaultInst;

extern "C" uint64_t CopyToUser_FaultInst;
void* const CopyToUser_FaultLocation = &CopyToUser_FaultInst;

#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED && defined(ARCHITECTURE_arm64)
__attribute__((naked)) bool VerifyIsReadableImpl(const void* Src, size_t Size) {
  __asm volatile(R"(
  // Early exit if size is zero.
  cbz x1, 2f;

  1:
  .globl UserReadable_FaultInst
  UserReadable_FaultInst:
  ldrb wzr, [x0], 1; // <- This line can fault.
  sub x1, x1, 1;
  cbnz x1, 1b;

  2:
  mov x0, 1;
  ret;
  )" ::
                   : "memory");
}

__attribute__((naked)) bool VerifyIsOnlyWritable(void* Src, size_t Size) {
  __asm volatile(R"(
  // Early exit if size is zero.
  cbz x1, 2f;

  1:
  ldrb w2, [x0];
  .globl UserWritable_FaultInst
  UserWritable_FaultInst:
  strb w2, [x0], 1; // <- This line can fault.

  sub x1, x1, 1;
  cbnz x1, 1b;

  2:
  mov x0, 1;
  ret;
  )" ::
                   : "memory");
}

__attribute__((naked)) bool VerifyIsStringReadableMaxSizeImpl(const char* Src, size_t MaxSize) {
  __asm volatile(R"(
  1:
  cbz x1, 2f;

  .globl UserStringReadable_FaultInst
  UserStringReadable_FaultInst:
  ldrb w2, [x0], 1; //< This line can fault.
  sub x1, x1, 1;
  cbnz x2, 1b;

  2:
  mov x0, 1;
  ret;
  )" ::
                   : "memory");
}

void VerifyIsReadable(const void* Src, size_t Size) {
  LOGMAN_THROW_A_FMT(VerifyIsReadableImpl(Src, Size), "EFAULT needs readable!");
}

void VerifyIsStringReadable(const char* Src) {
  LOGMAN_THROW_A_FMT(VerifyIsStringReadableMaxSizeImpl(Src, ~0ULL), "EFAULT needs string readable!");
}

void VerifyIsStringReadableMaxSize(const char* Src, size_t MaxSize) {
  LOGMAN_THROW_A_FMT(VerifyIsStringReadableMaxSizeImpl(Src, MaxSize), "EFAULT needs string readable!");
}

void VerifyIsReadableOrNull(const void* Src, size_t Size) {
  if (Src == nullptr) {
    return;
  }

  LOGMAN_THROW_A_FMT(VerifyIsReadableImpl(Src, Size), "EFAULT needs readable!");
}

void VerifyIsWritable(void* Src, size_t Size) {
  ///< Checking if writable needs to check if readable first.
  VerifyIsReadable(Src, Size);

  LOGMAN_THROW_A_FMT(VerifyIsOnlyWritable(Src, Size), "EFAULT needs writable!");
}

void VerifyIsWritableOrNull(void* Src, size_t Size) {
  if (Src == nullptr) {
    return;
  }

  ///< Checking if writable needs to check if readable first.
  VerifyIsReadable(Src, Size);
  LOGMAN_THROW_A_FMT(VerifyIsOnlyWritable(Src, Size), "EFAULT needs writable!");
}

extern "C" uint64_t UserReadable_FaultInst;
void* const UserReadable_FaultLocation = &UserReadable_FaultInst;

extern "C" uint64_t UserWritable_FaultInst;
void* const UserWritable_FaultLocation = &UserWritable_FaultInst;

extern "C" uint64_t UserStringReadable_FaultInst;
void* const UserStringReadable_FaultLocation = &UserStringReadable_FaultInst;
#endif

bool IsFaultLocation(uint64_t PC) {
  bool IsMemcpyFault = false;
  IsMemcpyFault |= reinterpret_cast<void*>(PC) == CopyToUser_FaultLocation;
  IsMemcpyFault |= reinterpret_cast<void*>(PC) == CopyFromUser_FaultLocation;
#if defined(ASSERTIONS_ENABLED) && ASSERTIONS_ENABLED && defined(ARCHITECTURE_arm64)
  IsMemcpyFault |= reinterpret_cast<void*>(PC) == UserReadable_FaultLocation;
  IsMemcpyFault |= reinterpret_cast<void*>(PC) == UserWritable_FaultLocation;
  IsMemcpyFault |= reinterpret_cast<void*>(PC) == UserStringReadable_FaultLocation;
#endif
  return IsMemcpyFault;
}

#else
size_t CopyFromUser(void* Dest, const void* Src, size_t Size) {
  memcpy(Dest, Src, Size);
  return Size;
}

size_t CopyToUser(void* Dest, const void* Src, size_t Size) {
  memcpy(Dest, Src, Size);
  return Size;
}

bool IsFaultLocation(uint64_t PC) {
  return false;
}
#endif
} // namespace FEX::HLE::FaultSafeUserMemAccess
