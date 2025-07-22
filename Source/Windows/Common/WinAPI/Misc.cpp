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

DLLEXPORT_FUNC(HMODULE, GetModuleHandleA, (LPCSTR lpModuleName)) {
  ScopedUnicodeString ModuleName {lpModuleName};
  return GetModuleHandleW(ModuleName->Buffer);
}

DLLEXPORT_FUNC(HMODULE, GetModuleHandleW, (LPCWSTR lpModuleName)) {
  HMODULE Res = nullptr;
  UNICODE_STRING ModuleName = InitUnicodeString(lpModuleName);

  NTSTATUS Status = LdrGetDllHandle(nullptr, 0, &ModuleName, &Res);
  if (Status) {
    SetLastError(RtlNtStatusToDosError(Status));
    return nullptr;
  }
  return Res;
}

DLLEXPORT_FUNC(FARPROC, GetProcAddress, (HMODULE hModule, LPCSTR lpProcName)) {
  void* Res = nullptr;
  STRING ProcName = InitAnsiString(lpProcName);
  NTSTATUS Status = LdrGetProcedureAddress(hModule, &ProcName, 0, &Res);
  if (Status) {
    SetLastError(RtlNtStatusToDosError(Status));
    return nullptr;
  }
  return reinterpret_cast<FARPROC>(Res);
}

DLLEXPORT_FUNC(void, RaiseException, (DWORD dwExceptionCode, DWORD dwExceptionFlags, DWORD nNumberOfArguments, CONST ULONG_PTR* lpArguments)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(WINBOOL, CloseHandle, (HANDLE hObject)) {
  return WinAPIReturn(NtClose(hObject));
}

DLLEXPORT_FUNC(WINBOOL, QueryPerformanceCounter, (LARGE_INTEGER * lpPerformanceCount)) {
  return RtlQueryPerformanceCounter(lpPerformanceCount);
}

DLLEXPORT_FUNC(WINBOOL, QueryPerformanceFrequency, (LARGE_INTEGER * lpFrequency)) {
  return RtlQueryPerformanceFrequency(lpFrequency);
}

DLLEXPORT_FUNC(void, GetSystemInfo, (LPSYSTEM_INFO lpSystemInfo)) {
  SYSTEM_BASIC_INFORMATION Info;

  if (NtQuerySystemInformation(SystemBasicInformation, &Info, sizeof(Info), nullptr)) {
    return;
  }

  *lpSystemInfo = SYSTEM_INFO {
    .wProcessorArchitecture = PROCESSOR_ARCHITECTURE_ARM64,
    .dwPageSize = Info.PhysicalPageSize,
    .lpMinimumApplicationAddress = reinterpret_cast<void*>(Info.LowestUserAddress),
    .lpMaximumApplicationAddress = reinterpret_cast<void*>(Info.HighestUserAddress),
    .dwActiveProcessorMask = Info.ActiveProcessors,
    .dwNumberOfProcessors = static_cast<BYTE>(Info.NumberOfProcessors),
    .dwProcessorType = 0,
    .dwAllocationGranularity = Info.AllocationGranularity,
    .wProcessorLevel = 0,
    .wProcessorRevision = 0,
  };
}

DLLEXPORT_FUNC(int, MultiByteToWideChar,
               (UINT CodePage, DWORD dwFlags, LPCCH lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar)) {
  DWORD Size = (cbMultiByte == -1) ? (strlen(lpMultiByteStr) + 1) : cbMultiByte;
  DWORD Res;
  if (!cchWideChar) {
    RtlMultiByteToUnicodeSize(&Res, lpMultiByteStr, Size);
  } else {
    RtlMultiByteToUnicodeN(lpWideCharStr, cchWideChar * sizeof(wchar_t), &Res, lpMultiByteStr, Size);
  }
  return static_cast<int>(Res / sizeof(wchar_t));
}

DLLEXPORT_FUNC(int, WideCharToMultiByte,
               (UINT CodePage, DWORD dwFlags, LPCWCH lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte,
                LPCCH lpDefaultChar, LPBOOL lpUsedDefaultChar)) {
  DWORD SizeW = ((cchWideChar == -1) ? (wcslen(lpWideCharStr) + 1) : cchWideChar) * sizeof(wchar_t);
  DWORD Res;
  if (!cbMultiByte) {
    RtlUnicodeToMultiByteSize(&Res, const_cast<wchar_t*>(lpWideCharStr), SizeW);
  } else {
    RtlUnicodeToMultiByteN(lpMultiByteStr, cbMultiByte, &Res, lpWideCharStr, SizeW);
  }
  if (lpUsedDefaultChar) {
    *lpUsedDefaultChar = false;
  }
  return static_cast<int>(Res);
}

DLLEXPORT_FUNC(DWORD, FormatMessageA,
               (DWORD dwFlags, const void* lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPSTR lpBuffer, DWORD nSize, va_list* Arguments)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(DWORD, FormatMessageW,
               (DWORD dwFlags, const void* lpSource, DWORD dwMessageId, DWORD dwLanguageId, LPWSTR lpBuffer, DWORD nSize, va_list* Arguments)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(DWORD, GetLastError, ()) {
  return GetCurrentTEB()->LastErrorValue;
}

DLLEXPORT_FUNC(void, SetLastError, (DWORD dwErrCode)) {
  GetCurrentTEB()->LastErrorValue = dwErrCode;
}

DLLEXPORT_FUNC(LONG, RegOpenKeyExA, (HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)) {
  if (hKey != HKEY_LOCAL_MACHINE) {
    UNIMPLEMENTED();
  }

  ScopedUnicodeString RootKey {"\\Registry\\Machine"};
  OBJECT_ATTRIBUTES ObjAttributes;
  InitializeObjectAttributes(&ObjAttributes, &*RootKey, OBJ_CASE_INSENSITIVE, nullptr, nullptr);
  HKEY HKLM;
  NTSTATUS Status = NtOpenKeyEx(reinterpret_cast<HANDLE*>(&HKLM), MAXIMUM_ALLOWED, &ObjAttributes, 0);
  if (Status) {
    return RtlNtStatusToDosError(Status);
  }

  ScopedUnicodeString SubKey {lpSubKey};
  InitializeObjectAttributes(&ObjAttributes, &*SubKey, OBJ_CASE_INSENSITIVE, reinterpret_cast<HANDLE>(HKLM), nullptr);
  Status = NtOpenKeyEx(reinterpret_cast<HANDLE*>(phkResult), samDesired, &ObjAttributes, ulOptions);
  NtClose(HKLM);
  return RtlNtStatusToDosError(Status);
}

DLLEXPORT_FUNC(LONG, RegGetValueA, (HKEY hKey, LPCSTR lpSubKey, LPCSTR lpValue, DWORD dwFlags, LPDWORD pdwType, PVOID pvData, LPDWORD pcbData)) {
  if (lpSubKey || dwFlags) {
    UNIMPLEMENTED();
  }

  ScopedUnicodeString ValueName {lpValue};

  union {
    KEY_VALUE_PARTIAL_INFORMATION Info;
    uint8_t Buf[512];
  } Data;
  ULONG OutSize;
  NTSTATUS Status = NtQueryValueKey(hKey, &*ValueName, KeyValuePartialInformation, &Data.Info, sizeof(Data), &OutSize);
  if (Status) {
    return RtlNtStatusToDosError(Status);
  }

  if (pdwType) {
    *pdwType = Data.Info.Type;
  }

  if (pvData) {
    if (*pcbData < Data.Info.DataLength) {
      *pcbData = Data.Info.DataLength;
      return ERROR_MORE_DATA;
    }

    memcpy(pvData, &Data.Info.Data, Data.Info.DataLength);
  }

  if (pcbData) {
    *pcbData = Data.Info.DataLength;
  }

  return ERROR_SUCCESS;
}

DLLEXPORT_FUNC(LONG, RegCloseKey, (HKEY hKey)) {
  return RtlNtStatusToDosError(NtClose(hKey));
}
