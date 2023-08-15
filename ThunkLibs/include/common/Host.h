/*
$info$
category: thunklibs ~ These are generated + glue logic 1:1 thunks unless noted otherwise
$end_info$
*/

#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

  __attribute__((weak))
  void MakeHostTrampolineForGuestFunctionAsyncCallable(HostToGuestTrampolinePtr*, unsigned AsyncWorkerThreadId);
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
struct [[deprecated]] fex_guest_function_ptr {
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

// Same as TrampolineInstanceInfo in Thunks.cpp
struct GuestcallInfo {
  uintptr_t HostPacker;
  void (*CallCallback)(uintptr_t GuestUnpacker, uintptr_t GuestTarget, void* argsrv);
  uintptr_t GuestUnpacker;
  uintptr_t GuestTarget;
  uintptr_t AsyncWorkerThread;
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
    bool is_passthrough = false;
    bool is_opaque = false;
};

// Generator emits specializations for this for each type that has compatible layout
template<typename T>
inline constexpr bool has_compatible_data_layout =
  std::is_integral_v<T> || std::is_enum_v<T> || std::is_floating_point_v<T>
#ifndef IS_32BIT_THUNK
  // If none of the previous predicates matched, the thunk generator did *not* emit a specialization for T.
  // This should not happen on 64-bit with the currently thunked libraries, since their types
  // * either have fully consistent data layout across 64-bit architectures.
  // * or use custom repacking, in which case has_compatible_data_layout isn't used
  //
  // Throwing a fake exception here will trigger a build failure.
  || (throw "Instantiated on a type that was expected to be compatible", true)
#endif
;

#ifndef IS_32BIT_THUNK
// Pointers have the same size, hence data layout compatibility only depends on the pointee type
template<typename T>
inline constexpr bool has_compatible_data_layout<T*> = has_compatible_data_layout<std::remove_cv_t<T>>;
template<typename T>
inline constexpr bool has_compatible_data_layout<T* const> = has_compatible_data_layout<std::remove_cv_t<T>*>;

// void* and void** are assumed to be compatible to simplify handling of libraries that use them ubiquitously
template<> inline constexpr bool has_compatible_data_layout<void*> = true;
template<> inline constexpr bool has_compatible_data_layout<const void*> = true;
template<> inline constexpr bool has_compatible_data_layout<void**> = true;
template<> inline constexpr bool has_compatible_data_layout<const void**> = true;
#endif

// Placeholder type to indicate the given data is in guest-layout
template<typename T>
struct guest_layout {
  static_assert(!std::is_class_v<T>, "No guest layout defined for this non-opaque struct type. This may be a bug in the thunk generator.");
  static_assert(!std::is_union_v<T>, "No guest layout defined for this non-opaque union type. This may be a bug in the thunk generator.");
  static_assert(!std::is_enum_v<T>, "No guest layout defined for this enum type. This is a bug in the thunk generator.");
  static_assert(!std::is_void_v<T>, "Attempted to get guest layout of void. Missing annotation for void pointer?");

  static_assert(std::is_fundamental_v<T> || has_compatible_data_layout<T>, "Default guest_layout may not be used for non-compatible data");

  using type = std::enable_if_t<!std::is_pointer_v<T>, T>;
  type data;

  // TODO: Make this conversion explicit
  guest_layout& operator=(const T from) {
    data = from;
    return *this;
  }

  // Allow conversion of integral types of same size and sign to each other.
  // This is useful for handling "long"/"long long" on 64-bit, as well as uint8_t/char.
  // TODO: Make this conversion explicit
  template<typename U>
  guest_layout& operator=(const guest_layout<U>& from) requires (std::is_integral_v<U> && sizeof(U) == sizeof(T) && std::is_convertible_v<T, U> && std::is_signed_v<T> == std::is_signed_v<U>) {
    data = static_cast<T>(from.data);
    return *this;
  }
};

#if IS_32BIT_THUNK
// Specialized for uint32_t so that members annotated as "size_t" can automatically be converted from 64-bit to 32-bit
template<>
struct guest_layout<uint32_t> {
  using type = uint32_t;
  type data;

  // TODO: Make this conversion explicit
  guest_layout& operator=(const uint32_t from) {
    data = from;
    return *this;
  }
  guest_layout& operator=(const guest_layout<size_t>& from) {
    if (from.data > 0xffffffff) {
      fprintf(stderr, "ERROR: Tried to truncate large size_t value passed across thunk boundaries\n");
      std::abort();
    }
    data = (uint32_t)from.data;
    return *this;
  }

  guest_layout() = default;
  guest_layout(const guest_layout<size_t>& from) {
    if (from.data > 0xffffffff) {
      fprintf(stderr, "ERROR: Tried to truncate large size_t value passed across thunk boundaries\n");
      std::abort();
    }
    data = (uint32_t)from.data;
  }
  guest_layout(const guest_layout& from) : data { from.data } {
  }
  guest_layout(uint32_t from) : data { from } {
  }
};
#endif

template<typename T, std::size_t N>
struct guest_layout<T[N]> {
  // TODO: Check that the underlying type is ABI compatible
//  static_assert(!std::is_class_v<T>, "No guest layout defined for this non-opaque struct type. This may be a bug in the thunk generator.");

  using type = std::enable_if_t<!std::is_pointer_v<T>, T>;
  std::array<guest_layout<type>, N> data;
};

template<typename T>
struct guest_layout<T*> {
#ifdef IS_32BIT_THUNK
  using type = uint32_t;
#else
  using type = uint64_t;
#endif
  type data;

  // TODO: Make this conversion explicit
  guest_layout& operator=(const T* from) {
    // TODO: Assert upper 32 bits are zero
    data = reinterpret_cast<uintptr_t>(from);
    return *this;
  }

  guest_layout<T>* get_pointer() {
    return reinterpret_cast<guest_layout<T>*>(uintptr_t { data });
  }

  const guest_layout<T>* get_pointer() const {
    return reinterpret_cast<const guest_layout<T>*>(uintptr_t { data });
  }
};

template<typename T>
struct guest_layout<T* const> {
#ifdef IS_32BIT_THUNK
  using type = uint32_t;
#else
  using type = uint64_t;
#endif
  type data;

  // TODO: Make this conversion explicit
  guest_layout& operator=(const T* from) {
    // TODO: Assert upper 32 bits are zero
    data = reinterpret_cast<uintptr_t>(from);
    return *this;
  }

  guest_layout<T>* get_pointer() {
    return reinterpret_cast<guest_layout<T>*>(uintptr_t { data });
  }

  const guest_layout<T>* get_pointer() const {
    return reinterpret_cast<const guest_layout<T>*>(uintptr_t { data });
  }
};

template<typename T>
struct host_layout;

template<typename T>
struct host_layout {
  static_assert(!std::is_class_v<T>, "No host_layout specialization generated for struct/class type");
  static_assert(!std::is_union_v<T>, "No host_layout specialization generated for union type");
  static_assert(!std::is_void_v<T>, "Attempted to get host layout of void. Missing annotation for void pointer?");

  // TODO: This generic implementation shouldn't be needed. Instead, auto-specialize host_layout for all types used as members.

  T data;

  host_layout(const guest_layout<T>& from) requires (!std::is_enum_v<T>) : data { from.data } {
    // NOTE: This is not strictly neccessary since differently sized types may
    //       be used across architectures. It's important that the host type
    //       can represent all guest values without loss, however.
    static_assert(sizeof(data) == sizeof(from));
  }

  host_layout(const guest_layout<T>& from) requires (std::is_enum_v<T>) : data { static_cast<T>(from.data) } {
  }

  // Allow conversion of integral types of same size and sign to each other.
  // This is useful for handling "long"/"long long" on 64-bit, as well as uint8_t/char.
  template<typename U>
  host_layout(const guest_layout<U>& from) requires (std::is_integral_v<U> && sizeof(U) == sizeof(T) && std::is_convertible_v<T, U> && std::is_signed_v<T> == std::is_signed_v<U>) : data { static_cast<T>(from.data) } {
  }

  host_layout(T from) requires (std::is_enum_v<T>) : data { from } {
  }
};

// Explicitly turn a host type into its corresponding host_layout
template<typename T>
const host_layout<T>& to_host_layout(const T& t) {
  static_assert(std::is_same_v<decltype(host_layout<T>::data), T>);
  return reinterpret_cast<const host_layout<T>&>(t);
}

// Specialization for size_t, which is 64-bit on 64-bit but 32-bit on 32-bit
template<>
struct host_layout<size_t> {
  size_t data;

  host_layout(const guest_layout<uint32_t>& from) : data { from.data } {
  }

  // TODO: Shouldn't be needed
  host_layout(const guest_layout<size_t>& from) : data { from.data } {
  }
};

template<typename T, size_t N>
struct host_layout<T[N]> {
  std::array<T, N> data;

  host_layout(const guest_layout<T[N]>& from) {
    for (size_t i = 0; i < N; ++i) {
      data[i] = host_layout<T> { from.data[i] }.data;
    }
  }
};

template<typename T>
struct host_layout<T*> {
  T* data;

  static_assert(!std::is_function_v<T>, "Function types must be handled separately");

  // Assume underlying data is compatible and just convert the guest-sized pointer to 64-bit
  host_layout(const guest_layout<T*>& from) : data { (T*)(uintptr_t)from.data } {
  }

  // TODO: Make this explicit?
  host_layout() = default;
};

template<typename T>
struct host_layout<T* const> {
  T* data;

  static_assert(!std::is_function_v<T>, "Function types must be handled separately");

  // Assume underlying data is compatible and just convert the guest-sized pointer to 64-bit
  host_layout(const guest_layout<T* const>& from) : data { (T*)(uintptr_t)from.data } {
  }
};

// Storage for unpacked parameters. May carry additional data for aiding conversion (such as short-lived, stack-allocated data instances to repoint pointer arguments to).
// Not suitable for nested struct repacking
// TODO: Update this comment. Originally it was written for unpacked_arg!
template<typename T>
struct unpacked_arg_base;
template<typename T>
struct unpacked_arg_base<T*> {
  unpacked_arg_base(host_layout<T>* data_) : data(data_) {
  }

  unpacked_arg_base(host_layout<const T>* data_) : data(data_) {
  }

  T* get() {
    static_assert(sizeof(T) == sizeof(host_layout<T>));
    static_assert(alignof(T) == alignof(host_layout<T>));
    return &data->data;
  }

  host_layout<T>* data;
};

// TODO: This shouldn't hold temporary storage for pointers that don't need repacking!
template<typename T>
struct unpacked_arg_with_storage;
template<typename T>
struct unpacked_arg_with_storage<T*> : unpacked_arg_base<T*> {
  // Silence -Wuninitialized warning from taking a pointer to opt
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wuninitialized"
  unpacked_arg_with_storage(const guest_layout<T*>& data_) : unpacked_arg_base<T*>(data_.get_pointer() ? &opt.extra : nullptr),
    opt {
      data_.get_pointer() ? Opt { *data_.get_pointer() }
      : Opt { .uninit = {} }
    } {
  }

  unpacked_arg_with_storage(const guest_layout<const T*>& data_) : unpacked_arg_base<T*>(data_.get_pointer() ? &opt.extra : nullptr),
  #pragma GCC diagnostic pop
    opt {
      data_.get_pointer() ? Opt { *data_.get_pointer() }
      : Opt { .uninit = {} }
    } {
  }

  // NOTE: Copying/moving this type would invalidate the reference to opt.extra,
  //       so disallow these operations altogether.
  unpacked_arg_with_storage(const unpacked_arg_with_storage& other) = delete;
  unpacked_arg_with_storage(unpacked_arg_with_storage&&) = delete;
  unpacked_arg_with_storage& operator=(const unpacked_arg_with_storage&) = delete;
  unpacked_arg_with_storage& operator=(unpacked_arg_with_storage&&) = delete;

  // Crude hack to allow leaving this uninitialized for nullptrs
  // NOTE: We can't use std::optional here since the storage must always be valid (even before initialization)
  union Opt {
    host_layout<T> extra; // Temporary storage for layout-repacked data
    char uninit;
  } opt;
};


template<>
struct unpacked_arg_with_storage<char*> {
  using type = char*;

  unpacked_arg_with_storage(/*const */guest_layout<const char*>& data) : data(reinterpret_cast<uint64_t>(data.get_pointer())) {
  }
  unpacked_arg_with_storage(/*const */guest_layout<char*>& data) : data(reinterpret_cast<uint64_t>(data.get_pointer())) {
  }

  type get() {
    return reinterpret_cast<type>(data);
  }

  uint64_t data;
};

template<typename T>
inline guest_layout<T> to_guest(const host_layout<T>& from) requires(!std::is_pointer_v<T>) {
  if constexpr (std::is_enum_v<T>) {
    // enums are represented by fixed-size integers in guest_layout, so explicitly cast them
    return guest_layout<T> { static_cast<std::underlying_type_t<T>>(from.data) };
  } else if constexpr (!std::is_same_v<T, uint32_t>) {
    guest_layout<T> ret { .data = from.data };
    return ret;
  } else {
    return guest_layout<uint32_t> { from.data };
  }
}

template<typename T>
inline guest_layout<T*> to_guest(const host_layout<T*>& from) {
  guest_layout<T*> ret;
  ret = from.data;
  return ret;
}

template<typename>
struct CallbackUnpack;

// Wrapper around unpacked_arg (for on-stack repacked argument structs) that implicitly converts to the represented type
template<typename T, typename GuestT>
struct unpacked_arg_wrapper {
  static_assert(std::is_pointer_v<T>);

  // Strip "const" from pointee type. Managing const-correctness would be a pain otherwise.
  using ActualT = std::add_pointer_t<std::remove_cv_t<std::remove_pointer_t<T>>>;

  unpacked_arg_with_storage<ActualT> data;
  guest_layout<T>& orig_arg;

  unpacked_arg_wrapper(guest_layout<GuestT>& orig_arg_) : data(orig_arg_), orig_arg(orig_arg_) {
  }

  ~unpacked_arg_wrapper() {
    // TODO: Properly detect opaque types
    if constexpr (requires(guest_layout<T> t, decltype(data) h) { t.get_pointer(); (bool)h.data; *data.data; }) {
      if constexpr (!std::is_const_v<std::remove_pointer_t<T>>) { // Skip exit-repacking for const pointees
        if (data.data) {
          constexpr bool is_compatible = has_compatible_data_layout<T> && std::is_same_v<T, GuestT>;
          if constexpr (!is_compatible && std::is_class_v<std::remove_pointer_t<T>>) {
            *orig_arg.get_pointer() = to_guest(*data.data); // TODO: Only if annotated as out-parameter
          }
        }
      }
    }
  }

  operator ActualT() {
    return data.get();
  }
};

template<typename T, ParameterAnnotations Annotation>
constexpr bool IsCompatible() {
  if constexpr (Annotation.is_opaque) {
    return true;
  } else if constexpr (has_compatible_data_layout<T>) {
    return true;
  } else {
    if constexpr (std::is_pointer_v<T>) {
      return has_compatible_data_layout<std::remove_cv_t<std::remove_pointer_t<T>>>;
    } else {
      return false;
    }
  }
}

template<ParameterAnnotations Annotation, typename HostT, typename T>
auto Projection(guest_layout<T>& data) {
  constexpr bool is_compatible = IsCompatible<T, Annotation>() && std::is_same_v<T, HostT>;
  if constexpr (Annotation.is_passthrough) {
    return data;
  } else if constexpr (is_compatible || !std::is_pointer_v<T>) {
    return host_layout<HostT> { data }.data;
  } else {
    // This argument requires temporary storage for repacked data
    // *and* it needs to call custom repack functions (if any)
    return unpacked_arg_wrapper<HostT, T> { data };
  }
}

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

template<typename, typename...>
struct GuestWrapperForHostFunction;

template<typename Result, typename... Args, typename... GuestArgs>
struct GuestWrapperForHostFunction<Result(Args...), GuestArgs...> {
  // Host functions called from Guest
  // NOTE: GuestArgs typically matches up with Args, however there may be exceptions (e.g. size_t)
  template<ParameterAnnotations... Annotations>
  static void Call(void* argsv) {
    static_assert(sizeof...(Annotations) == sizeof...(Args));
    static_assert(sizeof...(GuestArgs) == sizeof...(Args));

    auto args = reinterpret_cast<PackedArguments<Result, guest_layout<GuestArgs>..., uintptr_t>*>(argsv);
    constexpr auto CBIndex = sizeof...(GuestArgs);
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
    auto callback = reinterpret_cast<Result(*)(std::conditional_t<Annotations.is_passthrough, guest_layout<GuestArgs>, Args>..., uintptr_t)>(cb);

    auto f = [&callback](guest_layout<GuestArgs>... args, uintptr_t target) -> Result {
      // Fold over each of Annotations, Args, and args. This will match up the elements in triplets.
      return callback(Projection<Annotations, Args>(args)..., target);
    };
    Invoke(f, *args);
  }
};

// TODO: Move?
template<typename F>
void FinalizeHostTrampolineForGuestFunction(const guest_layout<F*>& PreallocatedTrampolineForGuestFunction) {
  FEXCore::FinalizeHostTrampolineForGuestFunction(
      (FEXCore::HostToGuestTrampolinePtr*)PreallocatedTrampolineForGuestFunction.data,
      (void*)&CallbackUnpack<F>::CallGuestPtr);
}

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
