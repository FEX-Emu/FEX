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
#include <optional>

#include "PackedArguments.h"

// Import FEX::HLE functions for use in host thunk libraries.
//
// Note these are statically linked into the FEX executable. The linker hence
// doesn't know about them when linking thunk libraries. This issue is avoided
// by declaring the functions as weak symbols.
namespace FEX::HLE {
struct HostToGuestTrampolinePtr;

__attribute__((weak)) HostToGuestTrampolinePtr* MakeHostTrampolineForGuestFunction(void* HostPacker, uintptr_t GuestTarget, uintptr_t GuestUnpacker);

__attribute__((weak)) HostToGuestTrampolinePtr* FinalizeHostTrampolineForGuestFunction(HostToGuestTrampolinePtr*, void* HostPacker);

__attribute__((weak)) void* GetGuestStack();

__attribute__((weak)) void MoveGuestStack(uintptr_t NewAddress);
} // namespace FEX::HLE

template<typename Fn>
struct function_traits;
template<typename Result, typename Arg>
struct function_traits<Result (*)(Arg)> {
  using result_t = Result;
  using arg_t = Arg;
};

template<auto Fn>
static typename function_traits<decltype(Fn)>::result_t fexfn_type_erased_unpack(void* argsv) {
  using args_t = typename function_traits<decltype(Fn)>::arg_t;
  return Fn(reinterpret_cast<args_t>(argsv));
}

struct ExportEntry {
  uint8_t* sha256;
  void (*fn)(void*);
};

typedef void fex_call_callback_t(uintptr_t callback, void* arg0, void* arg1);

#define EXPORTS(name)                       \
  extern "C" {                              \
  ExportEntry* fexthunks_exports_##name() { \
    if (!fexldr_init_##name()) {            \
      return nullptr;                       \
    }                                       \
    return exports;                         \
  }                                         \
  }

#define LOAD_LIB_INIT(init_fn)                         \
  __attribute__((constructor)) static void loadlib() { \
    init_fn();                                         \
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
#define LOAD_INTERNAL_GUESTPTR_VIA_CUSTOM_ABI(target_variable) asm volatile("mov %%r11, %0" : "=r"(target_variable))
#elif defined(_M_ARM_64)
#define LOAD_INTERNAL_GUESTPTR_VIA_CUSTOM_ABI(target_variable) asm volatile("mov %0, x11" : "=r"(target_variable))
#endif

struct ParameterAnnotations {
  bool is_passthrough = false;
  bool assume_compatible = false;
};

// Generator emits specializations for this for each type that has compatible layout
template<typename T>
inline constexpr bool has_compatible_data_layout =
  std::is_integral_v<T> || std::is_enum_v<T> ||
  std::is_floating_point_v<T>
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
template<>
inline constexpr bool has_compatible_data_layout<void*> = true;
template<>
inline constexpr bool has_compatible_data_layout<const void*> = true;
template<>
inline constexpr bool has_compatible_data_layout<void**> = true;
template<>
inline constexpr bool has_compatible_data_layout<const void**> = true;
#endif

// Placeholder type to indicate the given data is in guest-layout
template<typename T>
struct __attribute__((packed)) guest_layout {
  static_assert(!std::is_class_v<T>, "No guest layout defined for this non-opaque struct type. This may be a bug in the thunk generator.");
  static_assert(!std::is_union_v<T>, "No guest layout defined for this non-opaque union type. This may be a bug in the thunk generator.");
  static_assert(!std::is_enum_v<T>, "No guest layout defined for this enum type. This is a bug in the thunk generator.");
  static_assert(!std::is_void_v<T>, "Attempted to get guest layout of void. Missing annotation for void pointer?");

  static_assert(std::is_fundamental_v<T> || has_compatible_data_layout<T>, "Default guest_layout may not be used for non-compatible data");

  using type = std::enable_if_t<!std::is_pointer_v<T>, T>;
  type data;

  guest_layout& operator=(const T from) {
    data = from;
    return *this;
  }
};

template<typename T, std::size_t N>
struct __attribute__((packed)) guest_layout<T[N]> {
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

  // Allow implicit conversion for function pointers, since they disallow use of host_layout
  guest_layout& operator=(const T* from) requires (std::is_function_v<T>)
  {
    // TODO: Assert upper 32 bits are zero
    data = reinterpret_cast<uintptr_t>(from);
    return *this;
  }

  guest_layout<T>* get_pointer() {
    return reinterpret_cast<guest_layout<T>*>(uintptr_t {data});
  }

  const guest_layout<T>* get_pointer() const {
    return reinterpret_cast<const guest_layout<T>*>(uintptr_t {data});
  }

  T* force_get_host_pointer() {
    return reinterpret_cast<T*>(uintptr_t {data});
  }

  const T* force_get_host_pointer() const {
    return reinterpret_cast<const T*>(uintptr_t {data});
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

  // Allow implicit conversion for function pointers, since they disallow use of host_layout
  guest_layout& operator=(const T* from) requires (std::is_function_v<T>)
  {
    // TODO: Assert upper 32 bits are zero
    data = reinterpret_cast<uintptr_t>(from);
    return *this;
  }

  guest_layout<T>* get_pointer() {
    return reinterpret_cast<guest_layout<T>*>(uintptr_t {data});
  }

  const guest_layout<T>* get_pointer() const {
    return reinterpret_cast<const guest_layout<T>*>(uintptr_t {data});
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

  explicit host_layout(const guest_layout<T>& from) requires (!std::is_enum_v<T>)
    : data {from.data} {
    // NOTE: This is not strictly neccessary since differently sized types may
    //       be used across architectures. It's important that the host type
    //       can represent all guest values without loss, however.
    static_assert(sizeof(data) == sizeof(from));
  }

  explicit host_layout(const guest_layout<T>& from) requires (std::is_enum_v<T>)
    : data {static_cast<T>(from.data)} {}

  // Allow conversion of integral types of smaller or equal size and same sign
  // to each other. Zero-extension is applied if needed.
  // Notably, this is useful for handling "long"/"long long" on 64-bit, as well
  // as uint8_t/char.
  template<typename U>
  explicit host_layout(const guest_layout<U>& from)
    requires (std::is_integral_v<U> && sizeof(U) <= sizeof(T) && std::is_convertible_v<T, U> && std::is_signed_v<T> == std::is_signed_v<U>)
    : data {static_cast<T>(from.data)} {}
};

// Explicitly turn a host type into its corresponding host_layout
template<typename T>
const host_layout<T>& to_host_layout(const T& t) {
  static_assert(std::is_same_v<decltype(host_layout<T>::data), T>);
  return reinterpret_cast<const host_layout<T>&>(t);
}

template<typename T, size_t N>
struct host_layout<T[N]> {
  std::array<T, N> data;

  explicit host_layout(const guest_layout<T[N]>& from) {
    for (size_t i = 0; i < N; ++i) {
      data[i] = host_layout<T> {from.data[i]}.data;
    }
  }
};

template<typename T>
constexpr bool is_long_or_longlong =
  std::is_same_v<T, long> || std::is_same_v<T, unsigned long> || std::is_same_v<T, long long> || std::is_same_v<T, unsigned long long>;

template<typename T>
struct host_layout<T*> {
  T* data;

  static_assert(!std::is_function_v<T>, "Function types must be handled separately");

  // Assume underlying data is compatible and just convert the guest-sized pointer to 64-bit
  explicit host_layout(const guest_layout<T*>& from)
    : data {(T*)(uintptr_t)from.data} {}

  host_layout() = default;

  // Allow conversion of pointers to 64-bit integer types to "(un)signed long (long)*".
  // This is useful for handling "long"/"long long" on 64-bit, which are distinct types
  // but have equal data layout.
  template<typename U>
  explicit host_layout(const guest_layout<U*>& from)
    requires (is_long_or_longlong<std::remove_cv_t<T>> && std::is_integral_v<U> && std::is_convertible_v<T, U> &&
              std::is_signed_v<T> == std::is_signed_v<U>
#if __clang_major__ >= 16
              // Old clang versions don't support using sizeof on incomplete types when evaluating requires()
              && sizeof(T) == sizeof(U)
#endif
                )
    : data {(T*)(uintptr_t)from.data} {
  }

  // Allow conversion of pointers to 8-bit integer types to "char*".
  // This is useful since "char"/"signed char"/"unsigned char"/"int8_t"/"uint8_t"
  // may all be distinct types but have equal data layout
  template<typename U>
  explicit host_layout(const guest_layout<U*>& from)
    requires (std::is_same_v<std::remove_cv_t<T>, char> && std::is_integral_v<U> && std::is_convertible_v<T, U> && sizeof(U) == 1)
    : data {(T*)(uintptr_t)from.data} {}

  // Allow conversion of pointers to 32-bit integer types to "wchar_t*".
  template<typename U>
  explicit host_layout(const guest_layout<U*>& from) requires (
    std::is_same_v<std::remove_cv_t<T>, wchar_t> && std::is_integral_v<U> && std::is_convertible_v<T, U> && sizeof(U) == sizeof(wchar_t))
    : data {(T*)(uintptr_t)from.data} {}
};

template<typename T>
struct host_layout<T* const> {
  T* data;

  static_assert(!std::is_function_v<T>, "Function types must be handled separately");

  // Assume underlying data is compatible and just convert the guest-sized pointer to 64-bit
  explicit host_layout(const guest_layout<T* const>& from)
    : data {(T*)(uintptr_t)from.data} {}
};

// Wrapper around host_layout that repacks from a guest_layout on construction
// and exit-repacks on scope exit (if needed). The wrapper manages the storage
// needed for repacked data itself.
// This also implicitly converts to a pointer of the wrapped host type, since
// this conversion is required at all call sites anyway
template<typename T, typename GuestT>
struct repack_wrapper {
  static_assert(std::is_pointer_v<T>);

  // Strip "const" from pointee type in host_layout storage
  using PointeeT = std::remove_cv_t<std::remove_pointer_t<T>>;

  std::optional<host_layout<PointeeT>> data;
  guest_layout<GuestT>& orig_arg;

  repack_wrapper(guest_layout<GuestT>& orig_arg_)
    : orig_arg(orig_arg_) {
    if (orig_arg.get_pointer()) {
      data = {*orig_arg_.get_pointer()};

      if constexpr (!std::is_enum_v<T>) {
        constexpr bool is_compatible = has_compatible_data_layout<T> && std::is_same_v<T, GuestT>;
        if constexpr (!is_compatible && std::is_class_v<std::remove_pointer_t<T>>) {
          fex_apply_custom_repacking_entry(*data, *orig_arg_.get_pointer());
        }
      }
    }
  }

  ~repack_wrapper() {
    // TODO: Properly detect opaque types
    if constexpr (std::is_class_v<std::remove_pointer_t<T>> && requires(guest_layout<T> t, decltype(data) h) {
                    t.get_pointer();
                    (bool)h;
                    *data;
                  }) {
      if (data) {
        // NOTE: It's assumed that the native host library didn't modify any
        //       const-pointees, so we skip automatic exit repacking for them.
        //       However, *custom* repacking must still be applied since it
        //       might have unrelated side effects (such as deallocation of
        //       memory reserved on entry)
        if (!fex_apply_custom_repacking_exit(*orig_arg.get_pointer(), *data)) {
          if constexpr (!std::is_const_v<std::remove_pointer_t<T>>) { // Skip exit-repacking for const pointees
            if constexpr (!(has_compatible_data_layout<T> && std::is_same_v<T, GuestT>)) {
              *orig_arg.get_pointer() = to_guest(*data); // TODO: Only if annotated as out-parameter
            }
          }
        }
      }
    }
  }

  operator PointeeT*() {
    static_assert(sizeof(PointeeT) == sizeof(host_layout<PointeeT>));
    static_assert(alignof(PointeeT) == alignof(host_layout<PointeeT>));
    return data ? &data.value().data : nullptr;
  }
};

template<typename T, typename GuestT>
static repack_wrapper<T, GuestT> make_repack_wrapper(guest_layout<GuestT>& orig_arg) {
  return {orig_arg};
}

template<typename T>
T& unwrap_host(host_layout<T>& val) {
  return val.data;
}

template<typename T, typename T2>
T* unwrap_host(repack_wrapper<T*, T2>& val) {
  return val;
}

template<typename T>
struct host_to_guest_convertible {
  const host_layout<T>& from;

  // Conversion from host to guest layout for non-pointers
  operator guest_layout<T>() const requires (!std::is_pointer_v<T>)
  {
    if constexpr (std::is_enum_v<T>) {
      // enums are represented by fixed-size integers in guest_layout, so explicitly cast them
      return guest_layout<T> {static_cast<std::underlying_type_t<T>>(from.data)};
    } else {
      guest_layout<T> ret {.data = from.data};
      return ret;
    }
  }

  operator guest_layout<T>() const requires (std::is_pointer_v<T>)
  {
    // TODO: Assert upper 32 bits are zero
    guest_layout<T> ret;
    ret.data = reinterpret_cast<uintptr_t>(from.data);
    return ret;
  }

#if IS_32BIT_THUNK
  // Allow size_t -> uint32_t conversions, since they are so common on 32-bit
  operator guest_layout<uint32_t>() const requires (std::is_same_v<T, size_t>)
  {
    return {static_cast<uint32_t>(from.data)};
  }

  // libGL also needs to allow long->int conversions for return values...
  operator guest_layout<int32_t>() const requires (std::is_same_v<T, long>)
  {
    return {static_cast<int32_t>(from.data)};
  }
#endif

  // Make guest_layout of "long long" and "long" interoperable, since they are
  // the same type as far as data layout is concerned.
  operator guest_layout<const unsigned long long*>() const requires (std::is_same_v<T, const unsigned long*>)
  {
    return (guest_layout<const unsigned long long*>)reinterpret_cast<const host_to_guest_convertible<const unsigned long long*>&>(*this);
  }

  // Make guest_layout of "char" and "uint8_t" interoperable
  operator guest_layout<const uint8_t*>() const requires (std::is_same_v<T, const char*>)
  {
    return (guest_layout<const uint8_t*>)reinterpret_cast<const host_to_guest_convertible<const uint8_t*>&>(*this);
  }

  operator guest_layout<uint8_t*>() const requires (std::is_same_v<T, char*>)
  {
    return (guest_layout<uint8_t*>)reinterpret_cast<const host_to_guest_convertible<uint8_t*>&>(*this);
  }

  // Make guest_layout of "wchar_t" and "uint32_t" interoperable
  operator guest_layout<uint32_t*>() const requires (std::is_same_v<T, wchar_t*>)
  {
    return (guest_layout<uint32_t*>)reinterpret_cast<const host_to_guest_convertible<uint32_t*>&>(*this);
  }

  static_assert(sizeof(wchar_t) == 4);

  // Allow conversion of integral types of same size and sign to each other.
  // This is useful for handling "long"/"long long" on 64-bit, as well as uint8_t/char.
  template<typename U>
  operator guest_layout<U>() const
    requires (std::is_integral_v<U> && sizeof(U) == sizeof(T) && std::is_convertible_v<T, U> && std::is_signed_v<T> == std::is_signed_v<U>)
  {
    return guest_layout<U> {.data {static_cast<T>(from.data)}};
  }
};

template<typename T>
inline host_to_guest_convertible<T> to_guest(const host_layout<T>& from) {
  return {from};
}

template<typename>
struct CallbackUnpack;

template<typename T, ParameterAnnotations Annotation>
constexpr bool IsCompatible() {
  if constexpr (Annotation.assume_compatible) {
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

template<typename T>
struct decaying_host_layout {
  host_layout<T> data;
  operator T() {
    return data.data;
  }
};

template<ParameterAnnotations Annotation, typename HostT, typename T>
auto Projection(guest_layout<T>& data) {
  if constexpr (Annotation.is_passthrough) {
    return data;
  } else if constexpr ((IsCompatible<T, Annotation>() && std::is_same_v<T, HostT>) || !std::is_pointer_v<T>) {
    // Instead of using host_layout<HostT> { data }.data, return a wrapper object.
    // This ensures that temporary lifetime extension can kick in at call-site.
    return decaying_host_layout<HostT> {.data {data}};
  } else {
    // This argument requires temporary storage for repacked data
    // *and* it needs to call custom repack functions (if any)
    return make_repack_wrapper<HostT>(data);
  }
}

#ifdef IS_32BIT_THUNK
/**
 * Helper class to manage guest stack memory from a host function.
 *
 * The current guest stack position is saved upon construction and bumped
 * for each object construction. Upon destruction, the old guest stack is
 * restored.
 */
class GuestStackBumpAllocator final {
  uintptr_t Top = reinterpret_cast<uintptr_t>(FEX::HLE::GetGuestStack());
  uintptr_t Next = Top;

public:
  ~GuestStackBumpAllocator() {
    FEX::HLE::MoveGuestStack(Top);
  }

  template<typename T, typename... Args>
  T* New(Args&&... args) {
    Next -= sizeof(T);
    Next &= ~uintptr_t {alignof(T) - 1};
    FEX::HLE::MoveGuestStack(Next);
    return new (reinterpret_cast<void*>(Next)) T {std::forward<Args>(args)...};
  }
};
#endif

template<typename Result, typename... Args>
struct CallbackUnpack<Result(Args...)> {
  static Result CallGuestPtr(Args... args) {
    GuestcallInfo* guestcall;
    LOAD_INTERNAL_GUESTPTR_VIA_CUSTOM_ABI(guestcall);

#ifndef IS_32BIT_THUNK
    PackedArguments<Result, guest_layout<Args>...> packed_args = {to_guest(to_host_layout(args))...};
#else
    GuestStackBumpAllocator GuestStack;
    auto& packed_args = *GuestStack.New<PackedArguments<Result, guest_layout<Args>...>>(to_guest(to_host_layout(args))...);
#endif
    guestcall->CallCallback(guestcall->GuestUnpacker, guestcall->GuestTarget, &packed_args);

    if constexpr (!std::is_void_v<Result>) {
      return packed_args.rv;
    }
  }
};

template<bool Cond, typename T, typename GuestT>
using as_guest_layout_if = std::conditional_t<Cond, guest_layout<GuestT>, T>;

template<typename, typename...>
struct GuestWrapperForHostFunction;

template<typename Result, typename... Args, typename... GuestArgs>
struct GuestWrapperForHostFunction<Result(Args...), GuestArgs...> {
  // Host functions called from Guest
  // NOTE: GuestArgs typically matches up with Args, however there may be exceptions (e.g. size_t)
  template<ParameterAnnotations RetAnnotations, ParameterAnnotations... Annotations>
  static void Call(void* argsv) {
    static_assert(sizeof...(Annotations) == sizeof...(Args));
    static_assert(sizeof...(GuestArgs) == sizeof...(Args));

    auto args =
      reinterpret_cast<PackedArguments<as_guest_layout_if<!std::is_void_v<Result>, Result, Result>, guest_layout<GuestArgs>..., uintptr_t>*>(argsv);
    constexpr auto CBIndex = sizeof...(GuestArgs);
    uintptr_t cb;
    static_assert(CBIndex <= 18 || CBIndex == 23);
    if constexpr (CBIndex == 0) {
      cb = args->a0;
    } else if constexpr (CBIndex == 1) {
      cb = args->a1;
    } else if constexpr (CBIndex == 2) {
      cb = args->a2;
    } else if constexpr (CBIndex == 3) {
      cb = args->a3;
    } else if constexpr (CBIndex == 4) {
      cb = args->a4;
    } else if constexpr (CBIndex == 5) {
      cb = args->a5;
    } else if constexpr (CBIndex == 6) {
      cb = args->a6;
    } else if constexpr (CBIndex == 7) {
      cb = args->a7;
    } else if constexpr (CBIndex == 8) {
      cb = args->a8;
    } else if constexpr (CBIndex == 9) {
      cb = args->a9;
    } else if constexpr (CBIndex == 10) {
      cb = args->a10;
    } else if constexpr (CBIndex == 11) {
      cb = args->a11;
    } else if constexpr (CBIndex == 12) {
      cb = args->a12;
    } else if constexpr (CBIndex == 13) {
      cb = args->a13;
    } else if constexpr (CBIndex == 14) {
      cb = args->a14;
    } else if constexpr (CBIndex == 15) {
      cb = args->a15;
    } else if constexpr (CBIndex == 16) {
      cb = args->a16;
    } else if constexpr (CBIndex == 17) {
      cb = args->a17;
    } else if constexpr (CBIndex == 18) {
      cb = args->a18;
    } else if constexpr (CBIndex == 23) {
      cb = args->a23;
    }

    // This is almost the same type as "Result func(Args..., uintptr_t)", but
    // individual types annotated as passthrough are wrapped in guest_layout<>
    auto callback = reinterpret_cast<as_guest_layout_if<RetAnnotations.is_passthrough, Result, Result> (*)(
      as_guest_layout_if<Annotations.is_passthrough, Args, GuestArgs>..., uintptr_t)>(cb);

    auto f = [&callback](guest_layout<GuestArgs>... args, uintptr_t target) {
      // Fold over each of Annotations, Args, and args. This will match up the elements in triplets.
      if constexpr (std::is_void_v<Result>) {
        callback(Projection<Annotations, Args>(args)..., target);
      } else if constexpr (!RetAnnotations.is_passthrough) {
        return (guest_layout<Result>)to_guest(to_host_layout(callback(Projection<Annotations, Args>(args)..., target)));
      } else {
        return callback(Projection<Annotations, Args>(args)..., target);
      }
    };
    Invoke(f, *args);
  }
};

template<typename FuncType>
void MakeHostTrampolineForGuestFunctionAt(uintptr_t GuestTarget, uintptr_t GuestUnpacker, FuncType** Func) {
  *Func = (FuncType*)FEX::HLE::MakeHostTrampolineForGuestFunction((void*)&CallbackUnpack<FuncType>::CallGuestPtr, GuestTarget, GuestUnpacker);
}

template<typename F>
void FinalizeHostTrampolineForGuestFunction(F* PreallocatedTrampolineForGuestFunction) {
  FEX::HLE::FinalizeHostTrampolineForGuestFunction((FEX::HLE::HostToGuestTrampolinePtr*)PreallocatedTrampolineForGuestFunction,
                                                   (void*)&CallbackUnpack<F>::CallGuestPtr);
}

template<typename F>
void FinalizeHostTrampolineForGuestFunction(guest_layout<F*> PreallocatedTrampolineForGuestFunction) {
  FEX::HLE::FinalizeHostTrampolineForGuestFunction((FEX::HLE::HostToGuestTrampolinePtr*)PreallocatedTrampolineForGuestFunction.data,
                                                   (void*)&CallbackUnpack<F>::CallGuestPtr);
}

// In the case of the thunk host_loader being the default, FEX need to use dlsym with RTLD_DEFAULT.
// If FEX queried the symbol object directly then it wouldn't follow symbol overriding rules.
//
// Common usecase is LD_PRELOAD with a library that defines some symbols.
// And then programs and libraries will pick up the preloaded symbols.
// ex: MangoHud overrides GLX and EGL symbols.
inline void* dlsym_default(void* handle, const char* symbol) {
  return dlsym(RTLD_DEFAULT, symbol);
}
