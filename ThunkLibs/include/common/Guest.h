#pragma once
#include <stdint.h>
#include <type_traits>

#include <cstring>

template<typename T>
struct HostWraper {
  HostWraper() { memset(this, 0, sizeof(*this)); }
  HostWraper(T value) {  memset(this, 0, sizeof(*this)); this->value = value; }
  union{
  T value;
  uint8_t pad[16];
  };
  operator T&() {  return value; }
};

#ifndef _M_ARM_64
#define MAKE_THUNK(lib, name, hash) \
  extern "C" int fexthunks_##lib##_##name(void *args); \
  asm(".text\nfexthunks_" #lib "_" #name ":\n.byte 0xF, 0x3F\n.byte " hash );
#else
// We're compiling for IDE integration, so provide a dummy-implementation that just calls an undefined function.
// The name of that function serves as an error message if this library somehow gets loaded at runtime.
extern "C" void BROKEN_INSTALL___TRIED_LOADING_AARCH64_BUILD_OF_GUEST_THUNK();
#define MAKE_THUNK(lib, name, hash) \
  extern "C" int fexthunks_##lib##_##name(void *args) { \
    BROKEN_INSTALL___TRIED_LOADING_AARCH64_BUILD_OF_GUEST_THUNK(); \
    return 0; \
  }
#endif

// Generated fexfn_pack_ symbols should be hidden by default, but clang does
// not support aliasing to static functions. Make them regular non-static
// functions on that compiler instead, hence.
#if defined(__clang__)
#define FEX_PACKFN_LINKAGE
#else
#define FEX_PACKFN_LINKAGE static
#endif

struct LoadlibArgs {
    HostWraper<const char *> Name;
    HostWraper<uintptr_t> CallbackThunks;
};

MAKE_THUNK(fex, loadlib, "0x27, 0x7e, 0xb7, 0x69, 0x5b, 0xe9, 0xab, 0x12, 0x6e, 0xf7, 0x85, 0x9d, 0x4b, 0xc9, 0xa2, 0x44, 0x46, 0xcf, 0xbd, 0xb5, 0x87, 0x43, 0xef, 0x28, 0xa2, 0x65, 0xba, 0xfc, 0x89, 0x0f, 0x77, 0x80")
MAKE_THUNK(fex, link_address_to_function, "0xe6, 0xa8, 0xec, 0x1c, 0x7b, 0x74, 0x35, 0x27, 0xe9, 0x4f, 0x5b, 0x6e, 0x2d, 0xc9, 0xa0, 0x27, 0xd6, 0x1f, 0x2b, 0x87, 0x8f, 0x2d, 0x35, 0x50, 0xea, 0x16, 0xb8, 0xc4, 0x5e, 0x42, 0xfd, 0x77")
MAKE_THUNK(fex, is_lib_loaded, "0xee, 0x57, 0xba, 0x0c, 0x5f, 0x6e, 0xef, 0x2a, 0x8c, 0xb5, 0x19, 0x81, 0xc9, 0x23, 0xe6, 0x51, 0xae, 0x65, 0x02, 0x8f, 0x2b, 0x5d, 0x59, 0x90, 0x6a, 0x7e, 0xe2, 0xe7, 0x1c, 0x33, 0x8a, 0xff")

#define LOAD_LIB_BASE(name, callback_unpacks, init_fn) \
  __attribute__((constructor)) static void loadlib() \
  { \
    LoadlibArgs args; args.Name.value = #name; args.CallbackThunks.value = (uintptr_t)(callback_unpacks); \
    fexthunks_fex_loadlib(&args); \
    if ((init_fn)) ((void(*)())init_fn)(); \
  }

#define LOAD_LIB(name) LOAD_LIB_BASE(name, nullptr, nullptr)
#define LOAD_LIB_INIT(name, init_fn) LOAD_LIB_BASE(name, nullptr, init_fn)
#define LOAD_LIB_WITH_CALLBACKS(name) LOAD_LIB_BASE(name, &callback_unpacks, nullptr)
#define LOAD_LIB_WITH_CALLBACKS_INIT(name, init_fn) LOAD_LIB_BASE(name, (uintptr_t)&callback_unpacks, init_fn)

inline void LinkAddressToFunction(uintptr_t addr, uintptr_t target) {
    struct args_t {
        HostWraper<uintptr_t> original_callee;
        HostWraper<uintptr_t> target_addr; // Function to call when branching to replaced_addr
    };
    args_t args;// = { addr, target };
    args.original_callee.value = addr;
    args.target_addr.value = target;
    fexthunks_fex_link_address_to_function(&args);
}

inline bool IsLibLoaded(const char *libname) {
  struct {
    HostWraper<const char *> Name;
    HostWraper<bool> rv;
  } argsrv;// = { libname };

  argsrv.Name.value = libname;

  fexthunks_fex_is_lib_loaded(&argsrv);

  return argsrv.rv;
}

#if BITS==64
  #define CUSTOM_ABI_HOST_ADDR \
    uintptr_t host_addr; \
    asm volatile("mov %%r11, %0" : "=r" (host_addr));
#else
  #define CUSTOM_ABI_HOST_ADDR \
    uintptr_t host_addr; \
    asm volatile("mov %%ecx, %0" : "=r" (host_addr));
#endif