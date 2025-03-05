// SPDX-License-Identifier: MIT
#define NTDDI_VERSION 0x0A000005
#define WINAPI
#define WINBASEAPI

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cerrno>
#include <winternl.h>
#include <windows.h>
#include <processenv.h>
#include "../Priv.h"

DLLEXPORT_FUNC(void*, VirtualAlloc, (void* lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)) {
  NTSTATUS Status = NtAllocateVirtualMemory(NtCurrentProcess(), &lpAddress, 0, &dwSize, flAllocationType, flProtect);
  if (Status) {
    SetLastError(RtlNtStatusToDosError(Status));
    return nullptr;
  }
  return lpAddress;
}

DLLEXPORT_FUNC(SIZE_T, VirtualQuery, (LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, SIZE_T dwLength)) {
  SIZE_T WrittenSize;
  NTSTATUS Status =
    NtQueryVirtualMemory(NtCurrentProcess(), lpAddress, MemoryBasicInformation, reinterpret_cast<void*>(lpBuffer), dwLength, &WrittenSize);
  if (Status) {
    SetLastError(RtlNtStatusToDosError(Status));
    return 0;
  }
  return WrittenSize;
}

DLLEXPORT_FUNC(WINBOOL, VirtualProtect, (void* lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect)) {
  return WinAPIReturn(NtProtectVirtualMemory(NtCurrentProcess(), &lpAddress, &dwSize, flNewProtect, lpflOldProtect));
}

DLLEXPORT_FUNC(void*, VirtualAlloc2,
               (HANDLE Process, void* BaseAddress, SIZE_T Size, ULONG AllocationType, ULONG PageProtection,
                MEM_EXTENDED_PARAMETER* ExtendedParameters, ULONG ParameterCount)) {
  NTSTATUS Status = NtAllocateVirtualMemoryEx(Process ? Process : NtCurrentProcess(), &BaseAddress, &Size, AllocationType, PageProtection,
                                              ExtendedParameters, ParameterCount);
  if (Status) {
    SetLastError(RtlNtStatusToDosError(Status));
    return nullptr;
  }
  return BaseAddress;
}

DLLEXPORT_FUNC(WINBOOL, VirtualFree, (void* lpAddress, SIZE_T dwSize, DWORD dwFreeType)) {
  return WinAPIReturn(NtFreeVirtualMemory(NtCurrentProcess(), &lpAddress, &dwSize, dwFreeType));
}

DLLEXPORT_FUNC(WINBOOL, FlushInstructionCache, (HANDLE hProcess, const void* lpBaseAddress, SIZE_T dwSize)) {
  return WinAPIReturn(NtFlushInstructionCache(hProcess, const_cast<void*>(lpBaseAddress), dwSize));
}

DLLEXPORT_FUNC(DWORD, FlsAlloc, (PFLS_CALLBACK_FUNCTION lpCallback)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(void*, FlsGetValue, (DWORD dwFlsIndex)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(WINBOOL, FlsSetValue, (DWORD dwFlsIndex, void* lpFlsData)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(WINBOOL, FlsFree, (DWORD dwFlsIndex)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(HLOCAL, LocalFree, (HLOCAL hMem)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(DWORD, TlsAlloc, ()) {
  RtlAcquirePebLock();

  // Cannot use expansion slots or FLS here, as they would be freed before FEX can cleanup.
  DWORD Slot = RtlFindClearBitsAndSet(GetCurrentPEB()->TlsBitmap, 1, 1);
  if (Slot != -1) {
    GetCurrentTEB()->TlsSlots[Slot] = nullptr;
  }
  RtlReleasePebLock();
  return Slot;
}

DLLEXPORT_FUNC(void*, TlsGetValue, (DWORD dwTlsIndex)) {
  return GetCurrentTEB()->TlsSlots[dwTlsIndex];
}

DLLEXPORT_FUNC(WINBOOL, TlsSetValue, (DWORD dwTlsIndex, void* lpTlsValue)) {
  GetCurrentTEB()->TlsSlots[dwTlsIndex] = lpTlsValue;
  return true;
}

DLLEXPORT_FUNC(WINBOOL, TlsFree, (DWORD dwTlsIndex)) {
  RtlAcquirePebLock();

  RtlClearBits(GetCurrentPEB()->TlsBitmap, dwTlsIndex, 1);
  NTSTATUS Status = NtSetInformationThread(NtCurrentThread(), ThreadZeroTlsCell, &dwTlsIndex, sizeof(dwTlsIndex));
  RtlReleasePebLock();
  return WinAPIReturn(Status);
}
