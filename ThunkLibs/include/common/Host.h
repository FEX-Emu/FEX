/*
$info$
category: thunklibs ~ These are generated + glue logic 1:1 thunks unless noted otherwise
$end_info$
*/

#pragma once
#include <stdint.h>

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

template<auto Obj>
static void fexfn_fetch_symbol(void* argsv) {
    *reinterpret_cast<decltype(Obj)>(argsv) = *Obj;
}

struct ExportEntry { uint8_t* sha256; void(*fn)(void *); };

typedef void fex_call_callback_t(uintptr_t callback, void *arg0, void* arg1);

static fex_call_callback_t* call_guest;

/**
 * Opaque wrapper around a guest function pointer.
 *
 * This prevents accidental calls to foreign function pointers while still
 * allowing us to label function pointers as such.
 */
struct fex_guest_function_ptr {
private:
    [[maybe_unused]] void* value = nullptr;

public:
    fex_guest_function_ptr() = default;

    template<typename Ret, typename... Args>
    fex_guest_function_ptr(Ret (*ptr)(Args...)) : value(reinterpret_cast<void*>(ptr)) {}
};

#define EXPORTS(name) \
  extern "C" { \
    ExportEntry* fexthunks_exports_##name(void *a0, uintptr_t a1) { \
      call_guest = (fex_call_callback_t*)a0; \
      if (!fexldr_init_##name()) { \
        return nullptr; \
      } \
      return exports; \
    } \
  }

#define EXPORTS_INIT(name, init_fn) \
  extern "C" { \
    ExportEntry* fexthunks_exports_##name(void *a0, uintptr_t a1) { \
      call_guest = (fex_call_callback_t*)a0; \
      if (!fexldr_init_##name()) { \
        return nullptr; \
      } \
      init_fn (); \
      return exports; \
    } \
  }

#define EXPORTS_WITH_CALLBACKS(name) \
  extern "C" { \
    ExportEntry* fexthunks_exports_##name(void *a0, uintptr_t a1) { \
      call_guest = (fex_call_callback_t*)a0; \
      (uintptr_t&)callback_unpacks = a1; \
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

