#pragma once
#include <stdint.h>
#include <type_traits>

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

#define MAKE_CALLBACK(lib, name, ...) \
  static uint8_t fexcallback_##lib##_##name[32] = { __VA_ARGS__ };

// Generated fexfn_pack_ symbols should be hidden by default, but clang does
// not support aliasing to static functions. Make them regular non-static
// functions on that compiler instead, hence.
#if defined(__clang__)
#define FEX_PACKFN_LINKAGE
#else
#define FEX_PACKFN_LINKAGE static
#endif

struct LoadlibArgs {
    const char *Name;
};


// fex internal thunk interface
MAKE_THUNK(fex, load_lib, "0xd1, 0x31, 0x17, 0x56, 0xe3, 0x85, 0x25, 0x2a, 0xae, 0x29, 0x5e, 0x3f, 0xc2, 0xe5, 0x15, 0xcb, 0x12, 0x0e, 0x48, 0xda, 0x01, 0x51, 0xf9, 0xfb, 0x81, 0x44, 0x8e, 0x1c, 0x62, 0xa7, 0xb5, 0x69") \
MAKE_THUNK(fex, is_lib_loaded, "0xe6, 0xa8, 0xec, 0x1c, 0x7b, 0x74, 0x35, 0x27, 0xe9, 0x4f, 0x5b, 0x6e, 0x2d, 0xc9, 0xa0, 0x27, 0xd6, 0x1f, 0x2b, 0x87, 0x8f, 0x2d, 0x35, 0x50, 0xea, 0x16, 0xb8, 0xc4, 0x5e, 0x42, 0xfd, 0x77") \
MAKE_THUNK(fex, link_host_address_to_guest_function, "0xe6, 0xa8, 0xec, 0x1c, 0x7b, 0x74, 0x35, 0x27, 0xe9, 0x4f, 0x5b, 0x6e, 0x2d, 0xc9, 0xa0, 0x27, 0xd6, 0x1f, 0x2b, 0x87, 0x8f, 0x2d, 0x35, 0x50, 0xea, 0x16, 0xb8, 0xc4, 0x5e, 0x42, 0xfd, 0x77") \
MAKE_THUNK(fex, host_trampoline_for_guestcall, "0xa2, 0xa1, 0x95, 0x64, 0xad, 0x6e, 0xa5, 0x32, 0xc5, 0xb2, 0xcb, 0x5b, 0x5d, 0x85, 0xec, 0x99, 0x46, 0x9d, 0x5a, 0xf4, 0xa5, 0x2f, 0xbe, 0xa3, 0x7b, 0x7d, 0xd1, 0x8e, 0x44, 0xa7, 0x81, 0xe8") \
MAKE_THUNK(fex, guestcall_target_for_host_trampoline, "0x04, 0x15, 0x13, 0xb2, 0x29, 0x32, 0xc8, 0xd8, 0x1a, 0xc1, 0xa7, 0xba, 0x09, 0xaa, 0x9e, 0xb4, 0x91, 0x0c, 0x68, 0x66, 0x62, 0xac, 0xa8, 0x80, 0x6e, 0xbb, 0xbe, 0xda, 0x15, 0xed, 0x55, 0x4a") \

#define LOAD_LIB_BASE(name, init_fn) \
  __attribute__((constructor)) static void loadlib() \
  { \
    LoadlibArgs args =  { #name }; \
    fexthunks_fex_load_lib(&args); \
    if ((init_fn)) ((void(*)())init_fn)(); \
  }

#define LOAD_LIB(name) LOAD_LIB_BASE(name, nullptr)
#define LOAD_LIB_INIT(name, init_fn) LOAD_LIB_BASE(name, init_fn)

inline void LinkHostAddressToGuestFunction(uintptr_t HostAddr, uintptr_t GuestFunction) {
    struct args_t {
        uintptr_t original_callee;
        uintptr_t target_addr; // Function to call when branching to replaced_addr
    };
    args_t args = { HostAddr, GuestFunction };
    fexthunks_fex_link_host_address_to_guest_function(&args);
}

template<typename Packer, typename Target>
inline Target *HostTrampolineForGuestcall(uint8_t HostPacker[32], Packer *GuestUnpacker, Target *GuestTarget) {
  struct {
    void *HostPacker;
    uintptr_t GuestUnpacker;
    uintptr_t GuestTarget;
    uintptr_t rv;
  } argsrv = { HostPacker, (uintptr_t)GuestUnpacker, (uintptr_t)GuestTarget };

  fexthunks_fex_host_trampoline_for_guestcall((void*)&argsrv);

  return (Target *)argsrv.rv;
}

template<typename Target>
inline Target *GuestcallTargetForHostTrampoline(Target *HostTrampoline) {
  struct {
    uintptr_t HostTrampoline;
    uintptr_t rv;
  } argsrv = { (uintptr_t)HostTrampoline };

  fexthunks_fex_guestcall_target_for_host_trampoline((void*)&argsrv);

  return (Target *)argsrv.rv;
}


#define CUSTOM_ABI_HOST_ADDR \
  uintptr_t host_addr; \
  asm("mov %%r11, %0" : "=r" (host_addr))
