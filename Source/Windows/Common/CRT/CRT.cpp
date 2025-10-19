// SPDX-License-Identifier: MIT
#include <iterator>
#include <windef.h>
#include <winternl.h>
#include <wine/debug.h>
#include <rpmalloc/rpmalloc.h>
#include "CRT.h"

extern "C" {
__attribute__((section(".CRT$FEXA"))) void (*FEXA)() = nullptr;
__attribute__((section(".CRT$FEXZ"))) void (*FEXZ)() = nullptr;
__attribute__((section(".CRT$XIA"))) void (*XIA)() = nullptr;
__attribute__((section(".CRT$XIZ"))) void (*XIZ)() = nullptr;
__attribute__((section(".CRT$XCA"))) void (*XCA)() = nullptr;
__attribute__((section(".CRT$XCZ"))) void (*XCZ)() = nullptr;
__attribute__((section(".CRT$XDA"))) void (*XDA)() = nullptr;
__attribute__((section(".CRT$XDZ"))) void (*XDZ)() = nullptr;
__attribute__((section(".CRT$XLA"))) void (*XLA)(HINSTANCE, DWORD, LPVOID*) = nullptr;
__attribute__((section(".CRT$XZA"))) void (*XLZ)(HINSTANCE, DWORD, LPVOID*) = nullptr;

uint64_t _tls_index;
extern void (*__CTOR_LIST__[])();
extern void (*__DTOR_LIST__[])();

BOOL DllMainCRTStartup(HMODULE Handle, DWORD Reason, LPVOID Reserved) {
  LdrDisableThreadCalloutsForDll(Handle);
  return true;
}
}
namespace {
template<typename TFuncIt, typename... TArgs>
void RunFuncArray(TFuncIt Begin, TFuncIt End, TArgs... Args) {
  for (auto It = Begin; It != End; It++) {
    if (*It) {
      (**It)(Args...);
    }
  }
}
} // namespace

namespace FEX::Windows {
void InitCRTProcess() {
  rpmalloc_initialize(nullptr);

  auto GNUCtorBegin = &__CTOR_LIST__[1];
  auto GNUCtorEnd = GNUCtorBegin;
  while (*GNUCtorEnd != nullptr) {
    GNUCtorEnd++;
  }

  RunFuncArray(&FEXA, &FEXZ);
  RunFuncArray(std::reverse_iterator(GNUCtorEnd), std::reverse_iterator(GNUCtorBegin));
  RunFuncArray(&XIA, &XIZ);
  RunFuncArray(&XCA, &XCZ);
  RunFuncArray(&XLA, &XLZ, nullptr, DLL_PROCESS_ATTACH, nullptr);
}

void InitCRTThread() {
  rpmalloc_thread_initialize();
  RunFuncArray(&XLA, &XLZ, nullptr, DLL_THREAD_ATTACH, nullptr);
}

void DeinitCRTThread() {
  RunFuncArray(&XLA, &XLZ, nullptr, DLL_THREAD_DETACH, nullptr);
  rpmalloc_thread_finalize();
}
} // namespace FEX::Windows
