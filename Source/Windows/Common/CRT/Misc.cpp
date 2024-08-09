// SPDX-License-Identifier: MIT
#define _SECIMP
#define _CRTIMP
#include <cstdlib>
#include <cstdint>
#include <unistd.h>
#include <wchar.h>
#include <windef.h>
#include <winternl.h>
#include <winbase.h>
#include "../Priv.h"

namespace {
char* Env;

void InitEnv() {
  RtlAcquirePebLock();
  auto ProcessParams = reinterpret_cast<RTL_USER_PROCESS_PARAMETERS64*>(NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters);
  wchar_t* EnvW = reinterpret_cast<wchar_t*>(ProcessParams->Environment);
  DWORD SizeW = 4;
  // The PEB environment is terminated by two null wchars.
  for (wchar_t* It = EnvW; It[0] != 0 || It[1] != 0; It++, SizeW += 2)
    ;
  DWORD Size;
  RtlUnicodeToMultiByteSize(&Size, EnvW, SizeW);
  Env = reinterpret_cast<char*>(RtlAllocateHeap(GetProcessHeap(), 0, Size + 1));
  RtlUnicodeToMultiByteN(Env, Size + 1, nullptr, EnvW, SizeW);
  RtlReleasePebLock();
}

__attribute__((used, section(".CRT$FEXB"))) void (*_InitEnv)(void) = InitEnv;
} // namespace

char* getenv(const char* VarName) {
  size_t VarNameLen = strlen(VarName);
  char* It = Env;
  char* Ret = nullptr;

  while (*It) {
    char* Eq = strchr(It, '=');
    if (Eq && Eq - It == VarNameLen && !strncmp(It, VarName, VarNameLen)) {
      Ret = Eq + 1;
      break;
    }

    It += strlen(It) + 1;
  }

  return Ret;
}

int atexit(void (*)(void)) {
  return 0;
}

#pragma push_macro("abort")
#undef abort
void abort(void) {
  UNIMPLEMENTED();
}
#pragma pop_macro("abort")

int getpid(void) {
  return static_cast<int>(GetCurrentProcessId());
}

void exit(int _Code) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(void, _assert, (const char* message, const char* file, unsigned line)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(
  uintptr_t, _beginthreadex,
  (void* security, unsigned stack_size, unsigned(__stdcall* start_address)(void*), void* arglist, unsigned initflag, unsigned* thrdaddr)) {
  UNIMPLEMENTED();
}

DLLEXPORT_FUNC(int*, __sys_nerr, (void)) {
  UNIMPLEMENTED();
}
