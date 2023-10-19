/*
$info$
category: thunklibs ~ These are generated + glue logic 1:1 thunks unless noted otherwise
$end_info$
*/

#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>

#include "PackedArguments.h"

// Import FEXCore functions for use in host thunk libraries.
//
// Note these are statically linked into the FEX executable. The linker hence
// doesn't know about them when linking thunk libraries. This issue is avoided
// by declaring the functions as weak symbols.
namespace FEXCore {
  struct HostToGuestTrampolinePtr;

  __attribute__((weak))
  HostToGuestTrampolinePtr*
  MakeHostTrampolineForGuestFunction(void* HostPacker, uintptr_t GuestTarget, uintptr_t GuestUnpacker);

  __attribute__((weak))
  HostToGuestTrampolinePtr*
  FinalizeHostTrampolineForGuestFunction(HostToGuestTrampolinePtr*, void* HostPacker);
}

template<typename Fn>
struct function_traits;
template<typename Result, typename Arg>
struct function_traits<Result(*)(Arg)> {
    using result_t = Result;
    using arg_t = Arg;
};

template<auto Fn>
static typename function_traits<decltype(Fn)>::result_t
fexfn_type_erased_unpack(void* argsv) {
    using args_t = typename function_traits<decltype(Fn)>::arg_t;
    return Fn(reinterpret_cast<args_t>(argsv));
}

struct ExportEntry { uint8_t* sha256; void(*fn)(void *); };

typedef void fex_call_callback_t(uintptr_t callback, void *arg0, void* arg1);

/**
 * Opaque wrapper around a guest function pointer.
 *
 * This prevents accidental calls to foreign function pointers while still
 * allowing us to label function pointers as such.
 */
struct fex_guest_function_ptr {
private:
    void* value = nullptr;

public:
    fex_guest_function_ptr() = default;

    template<typename Ret, typename... Args>
    fex_guest_function_ptr(Ret (*ptr)(Args...)) : value(reinterpret_cast<void*>(ptr)) {}

    inline operator bool() const {
      return value != nullptr;
    }
};

#define EXPORTS(name) \
  extern "C" { \
    ExportEntry* fexthunks_exports_##name() { \
      if (!fexldr_init_##name()) { \
        return nullptr; \
      } \
      return exports; \
    } \
  }

#define LOAD_LIB_INIT(init_fn) \
  __attribute__((constructor)) static void loadlib() \
  { \
    init_fn (); \
  }

struct GuestcallInfo {
  uintptr_t HostPacker;
  void (*CallCallback)(uintptr_t GuestUnpacker, uintptr_t GuestTarget, void* argsrv);
  uintptr_t GuestUnpacker;
  uintptr_t GuestTarget;
};

// Helper macro for reading an internal argument passed through the `r11`
// host register. This macro must be placed at the very beginning of
// the function it is used in.
#if defined(_M_X86_64)
#define LOAD_INTERNAL_GUESTPTR_VIA_CUSTOM_ABI(target_variable) \
  asm volatile("mov %%r11, %0" : "=r" (target_variable))
#elif defined(_M_ARM_64)
#define LOAD_INTERNAL_GUESTPTR_VIA_CUSTOM_ABI(target_variable) \
  asm volatile("mov %0, x11" : "=r" (target_variable))
#endif

struct ParameterAnnotations {
};

// Placeholder type to indicate the given data is in guest-layout
template<typename T>
struct guest_layout {
  T data;
};

template<typename>
struct CallbackUnpack;

template<typename Result, typename... Args>
struct CallbackUnpack<Result(Args...)> {
  static Result CallGuestPtr(Args... args) {
    GuestcallInfo *guestcall;
    LOAD_INTERNAL_GUESTPTR_VIA_CUSTOM_ABI(guestcall);

    PackedArguments<Result, Args...> packed_args = {
      args...
    };
    guestcall->CallCallback(guestcall->GuestUnpacker, guestcall->GuestTarget, &packed_args);
    if constexpr (!std::is_void_v<Result>) {
      return packed_args.rv;
    }
  }
};

template<ParameterAnnotations Annotation, typename T>
auto Projection(guest_layout<T>& data) {
  return data.data;
}

template<typename>
struct GuestWrapperForHostFunction;

template<typename Result, typename... Args>
struct GuestWrapperForHostFunction<Result(Args...)> {
  // Host functions called from Guest
  template<ParameterAnnotations... Annotations>
  static void Call(void* argsv) {
    static_assert(sizeof...(Annotations) == sizeof...(Args));

    auto args = reinterpret_cast<PackedArguments<Result, guest_layout<Args>..., uintptr_t>*>(argsv);
    constexpr auto CBIndex = sizeof...(Args);
    uintptr_t cb;
    static_assert(CBIndex <= 18 || CBIndex == 23);
    if constexpr(CBIndex == 0) {
      cb = args->a0;
    } else if constexpr(CBIndex == 1) {
      cb = args->a1;
    } else if constexpr(CBIndex == 2) {
      cb = args->a2;
    } else if constexpr(CBIndex == 3) {
      cb = args->a3;
    } else if constexpr(CBIndex == 4) {
      cb = args->a4;
    } else if constexpr(CBIndex == 5) {
      cb = args->a5;
    } else if constexpr(CBIndex == 6) {
      cb = args->a6;
    } else if constexpr(CBIndex == 7) {
      cb = args->a7;
    } else if constexpr(CBIndex == 8) {
      cb = args->a8;
    } else if constexpr(CBIndex == 9) {
      cb = args->a9;
    } else if constexpr(CBIndex == 10) {
      cb = args->a10;
    } else if constexpr(CBIndex == 11) {
      cb = args->a11;
    } else if constexpr(CBIndex == 12) {
      cb = args->a12;
    } else if constexpr(CBIndex == 13) {
      cb = args->a13;
    } else if constexpr(CBIndex == 14) {
      cb = args->a14;
    } else if constexpr(CBIndex == 15) {
      cb = args->a15;
    } else if constexpr(CBIndex == 16) {
      cb = args->a16;
    } else if constexpr(CBIndex == 17) {
      cb = args->a17;
    } else if constexpr(CBIndex == 18) {
      cb = args->a18;
    } else if constexpr(CBIndex == 23) {
      cb = args->a23;
    }

    // This is almost the same type as "Result func(Args..., uintptr_t)", but
    // individual parameters annotated as passthrough are replaced by guest_layout<GuestArgs>
    auto callback = reinterpret_cast<Result(*)(Args..., uintptr_t)>(cb);

    auto f = [&callback](guest_layout<Args>... args, uintptr_t target) -> Result {
      // Fold over each of Annotations, Args, and args. This will match up the elements in triplets.
      return callback(Projection<Annotations, Args>(args)..., target);
    };
    Invoke(f, *args);
  }
};

template<typename FuncType>
void MakeHostTrampolineForGuestFunctionAt(uintptr_t GuestTarget, uintptr_t GuestUnpacker, FuncType **Func) {
    *Func = (FuncType*)FEXCore::MakeHostTrampolineForGuestFunction(
        (void*)&CallbackUnpack<FuncType>::CallGuestPtr,
        GuestTarget,
        GuestUnpacker);
}

template<typename F>
void FinalizeHostTrampolineForGuestFunction(F* PreallocatedTrampolineForGuestFunction) {
  FEXCore::FinalizeHostTrampolineForGuestFunction(
      (FEXCore::HostToGuestTrampolinePtr*)PreallocatedTrampolineForGuestFunction,
      (void*)&CallbackUnpack<F>::CallGuestPtr);
}

// In the case of the thunk host_loader being the default, FEX need to use dlsym with RTLD_DEFAULT.
// If FEX queried the symbol object directly then it wouldn't follow symbol overriding rules.
//
// Common usecase is LD_PRELOAD with a library that defines some symbols.
// And then programs and libraries will pick up the preloaded symbols.
// ex: MangoHud overrides GLX and EGL symbols.
inline
void *dlsym_default(void* handle, const char* symbol) {
  return dlsym(RTLD_DEFAULT, symbol);
}
