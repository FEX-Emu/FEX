// SPDX-License-Identifier: MIT
#define NTDDI_VERSION 0x0A000005
#define WINAPI
#define WINBASEAPI

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cerrno>
#include <ntstatus.h>
#include <winternl.h>
#include <windows.h>
#include <processenv.h>
#include "../Priv.h"

WINBOOL WaitOnAddress(volatile void* Address, void* CompareAddress, SIZE_T AddressSize, DWORD dwMilliseconds) {
  return RtlWaitOnAddress(Address, CompareAddress, AddressSize, dwMilliseconds) == STATUS_SUCCESS;
}

void WakeByAddressAll(PVOID Address) {
  RtlWakeAddressAll(Address);
}

void WINAPI WakeByAddressSingle(PVOID Address) {
  RtlWakeAddressSingle(Address);
}

DLLEXPORT_FUNC(void, InitializeSRWLock, (PSRWLOCK SRWLock)) {
  RtlInitializeSRWLock(SRWLock);
}

void AcquireSRWLockExclusive(PSRWLOCK SRWLock) {
  RtlAcquireSRWLockExclusive(SRWLock);
}

void ReleaseSRWLockExclusive(PSRWLOCK SRWLock) {
  RtlReleaseSRWLockExclusive(SRWLock);
}

void AcquireSRWLockShared(PSRWLOCK SRWLock) {
  RtlAcquireSRWLockShared(SRWLock);
}

void ReleaseSRWLockShared(PSRWLOCK SRWLock) {
  RtlReleaseSRWLockShared(SRWLock);
}

DLLEXPORT_FUNC(BOOLEAN, TryAcquireSRWLockShared, (PSRWLOCK SRWLock)) {
  return RtlTryAcquireSRWLockShared(SRWLock);
}

DLLEXPORT_FUNC(BOOLEAN, TryAcquireSRWLockExclusive, (PSRWLOCK SRWLock)) {
  return RtlTryAcquireSRWLockExclusive(SRWLock);
}

DLLEXPORT_FUNC(void, InitializeCriticalSection, (LPCRITICAL_SECTION lpCriticalSection)) {
  RtlInitializeCriticalSection(lpCriticalSection);
}

DLLEXPORT_FUNC(void, EnterCriticalSection, (LPCRITICAL_SECTION lpCriticalSection)) {
  RtlEnterCriticalSection(lpCriticalSection);
}

DLLEXPORT_FUNC(void, LeaveCriticalSection, (LPCRITICAL_SECTION lpCriticalSection)) {
  RtlLeaveCriticalSection(lpCriticalSection);
}

DLLEXPORT_FUNC(WINBOOL, TryEnterCriticalSection, (LPCRITICAL_SECTION lpCriticalSection)) {
  return RtlTryEnterCriticalSection(lpCriticalSection);
}

DLLEXPORT_FUNC(void, DeleteCriticalSection, (LPCRITICAL_SECTION lpCriticalSection)) {
  RtlDeleteCriticalSection(lpCriticalSection);
}

DLLEXPORT_FUNC(void, InitializeConditionVariable, (PCONDITION_VARIABLE ConditionVariable)) {
  RtlInitializeConditionVariable(ConditionVariable);
}

DLLEXPORT_FUNC(void, WakeConditionVariable, (PCONDITION_VARIABLE ConditionVariable)) {
  RtlWakeConditionVariable(ConditionVariable);
}

DLLEXPORT_FUNC(void, WakeAllConditionVariable, (PCONDITION_VARIABLE ConditionVariable)) {
  RtlWakeAllConditionVariable(ConditionVariable);
}

DLLEXPORT_FUNC(WINBOOL, SleepConditionVariableSRW, (PCONDITION_VARIABLE ConditionVariable, PSRWLOCK SRWLock, DWORD dwMilliseconds, ULONG Flags)) {
  LARGE_INTEGER Time;
  // A negative value indicates a relative time measured in 100ns intervals.
  Time.QuadPart = static_cast<ULONGLONG>(dwMilliseconds) * -10000;
  return RtlSleepConditionVariableSRW(ConditionVariable, SRWLock, dwMilliseconds == INFINITE ? nullptr : &Time, Flags);
}

DLLEXPORT_FUNC(WINBOOL, InitOnceExecuteOnce, (PINIT_ONCE InitOnce, PINIT_ONCE_FN InitFn, void* Parameter, void** Context)) {
  return !RtlRunOnceExecuteOnce(InitOnce, reinterpret_cast<PRTL_RUN_ONCE_INIT_FN>(InitFn), Parameter, Context);
}

DLLEXPORT_FUNC(DWORD, WaitForSingleObjectEx, (HANDLE hHandle, DWORD dwMilliseconds, WINBOOL bAlertable)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(HANDLE, GetProcessHeap, ()) {
  return GetCurrentPEB()->ProcessHeap;
}

DLLEXPORT_FUNC(DWORD, GetCurrentProcessId, ()) {
  return static_cast<DWORD>(reinterpret_cast<uintptr_t>(GetCurrentTEB()->ClientId.UniqueProcess));
}

DLLEXPORT_FUNC(DWORD, GetCurrentThreadId, ()) {
  return static_cast<DWORD>(reinterpret_cast<uintptr_t>(GetCurrentTEB()->ClientId.UniqueThread));
}

DLLEXPORT_FUNC(DWORD, GetThreadId, (HANDLE Thread)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(HANDLE, GetCurrentProcess, ()) {
  return NtCurrentProcess();
}

DLLEXPORT_FUNC(HANDLE, GetCurrentThread, ()) {
  return NtCurrentThread();
}

DLLEXPORT_FUNC(DWORD, GetCurrentProcessorNumber, ()) {
  return NtGetCurrentProcessorNumber();
}

DLLEXPORT_FUNC(WINBOOL, SwitchToThread, ()) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(void, Sleep, (DWORD dwMilliseconds)) {
  UNIMPLEMENTED();
}
