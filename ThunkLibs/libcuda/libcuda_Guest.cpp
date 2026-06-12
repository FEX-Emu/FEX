// SPDX-License-Identifier: MIT
#include "common/Guest.h"
#include "cuda_defines.h"

#include "thunkgen_guest_libcuda.inl"
#include <cstdio>
#include <dlfcn.h>
#include <functional>
#include <string_view>
#include <unordered_map>

#include <cstdint>

// Maps cuda API function names to the address of a guest function which is
// linked to the corresponding host function pointer
const std::unordered_map<std::string_view, uintptr_t /* guest function address */> HostPtrInvokers = std::invoke([]() {
#define PAIR(name, unused) Ret[#name] = reinterpret_cast<uintptr_t>(GetCallerForHostFunction(name));
  std::unordered_map<std::string_view, uintptr_t> Ret;
  FOREACH_internal_SYMBOL(PAIR);
  return Ret;
#undef PAIR
});

extern "C" {

// This variable controls the behavior of cuGetProcAddress for functions we don't know the signature of:
// - if false (default), we return a nullptr (since the application might have a fallback code path)
// - if true, we return a stub function that fatally errors upon being called
constexpr bool stub_unknown_functions = false;

// Fatally erroring function with a thunk-like interface. This is used as a placeholder for unknown CUDA functions
[[noreturn]]
static void FatalError(void* raw_args) {
  auto called_function = reinterpret_cast<PackedArguments<void, uintptr_t>*>(raw_args)->a0;
  fprintf(stderr, "FATAL: Called unknown CUDA function at address %p\n", reinterpret_cast<void*>(called_function));
  __builtin_trap();
}

static void* MakeGuestCallable(const char* origin, void* func, const char* name) {
  auto It = HostPtrInvokers.find(name);
  if (It == HostPtrInvokers.end()) {
    fprintf(stderr, "%s: Unknown cuda function at address %p: %s\n", origin, func, name);
    if (stub_unknown_functions) {
      const auto StubHostPtrInvoker = CallHostFunction<FatalError, void>;
      LinkAddressToFunction((uintptr_t)func, reinterpret_cast<uintptr_t>(StubHostPtrInvoker));
      return func;
    }
    return nullptr;
  }
  LinkAddressToFunction((uintptr_t)func, It->second);
  return func;
}

struct override_entry {
  std::string_view name;
  void* ptr;
};

constexpr static std::array<override_entry, 2> proc_override = {
  {{"cuGetProcAddress", (void*)cuGetProcAddress_v2}, {"cuGetProcAddress_v2", (void*)cuGetProcAddress_v2}}};

CUresult cuGetProcAddress_v2(const char* symbol, void** pfn, int cudaVersion, cuuint64_t flags, CUdriverProcAddressQueryResult* symbolStatus) {

  for (auto& over : proc_override) {
    if (symbol == over.name) {
      *pfn = over.ptr;
      if (symbolStatus) {
        *symbolStatus = (CUdriverProcAddressQueryResult)0;
      }
      return (CUresult)0; // CUDA_SUCCESS
    }
  }

  void* ptr {};
  auto Ret = fexfn_pack_cuGetProcAddress_v2(symbol, &ptr, cudaVersion, flags, symbolStatus);

  if (!Ret) {
    *pfn = MakeGuestCallable(__FUNCTION__, ptr, symbol);
  }
  return Ret;
}
}

LOAD_LIB(libcuda)
