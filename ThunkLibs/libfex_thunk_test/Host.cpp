/*
$info$
tags: thunklibs|asound
$end_info$
*/

#include <dlfcn.h>

#include <chrono>
#include <cstring>
#include <new>
#include <thread>
#include <unordered_map>

#include "common/Host.h"

#include "api.h"

#include "thunkgen_host_libfex_thunk_test.inl"

#if 0
std::thread* thr;

void fexfn_impl_libfex_thunk_test_SetAsyncCallback(void(*cb)()) {
  auto host_cb = (void (*)())cb;
  FinalizeHostTrampolineForGuestFunction(host_cb);
  MakeHostTrampolineForGuestFunctionAsyncCallable(host_cb, 1);

  thr = new std::thread { [host_cb]() {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(500ms);
    host_cb();
  }};

//  host_cb();
  printf("HELLO WORLD\n");
}
#endif

using const_void_ptr = const void*;

template<StructType TypeIndex, typename Type>
static const TestBaseStruct* convert(const TestBaseStruct* source) {
    // Using malloc here since no easily available type information is available at the time of destruction
    auto child = (host_layout<Type>*)malloc(sizeof(host_layout<Type>));
    new (child) host_layout<Type> { *reinterpret_cast<guest_layout<Type>*>((void*)(source)) }; // TODO: Use proper cast?
    // TODO: Trigger *full* custom repack for children, not just the Next member
    child->data.Next = fex_custom_repack<&Type::Next>(child->data.Next);

    return (const TestBaseStruct*)child; // TODO: Use proper cast?
}

template<StructType TypeIndex, typename Type>
inline constexpr std::pair<StructType, const TestBaseStruct*(*)(const TestBaseStruct*)> converters =
  { TypeIndex, convert<TypeIndex, Type> };


static std::unordered_map<StructType, const TestBaseStruct*(*)(const TestBaseStruct*)> next_handlers {
    converters<StructType::Struct1, TestStruct1>,
    converters<StructType::Struct2, TestStruct2>,
};

// Normally, we would implement fex_custom_repack individually for each customized struct.
// In this case, they all need the same repacking, so we just implement it once and alias all fex_custom_repack instances
extern "C" const_void_ptr default_fex_custom_repack(const const_void_ptr& source) {
    if (!source) {
        return nullptr;
    }

    auto typed_source = reinterpret_cast<const TestBaseStruct*>(source);
    auto child = next_handlers.at(typed_source->Type)(typed_source);
    return child;
}

template<> __attribute__((alias("default_fex_custom_repack"))) const_void_ptr fex_custom_repack<&TestStruct1::Next>(const const_void_ptr&);
template<> __attribute__((alias("default_fex_custom_repack"))) const_void_ptr fex_custom_repack<&TestStruct2::Next>(const const_void_ptr&);

extern "C" void default_fex_custom_repack_postcall(const const_void_ptr& Next) {
  if (Next) {
    auto NextNext = ((TestBaseStruct*)Next)->Next;
    default_fex_custom_repack_postcall(NextNext);
    fprintf(stderr, "Destroying %p\n", Next);
    free((void*)Next);
  }
}

template<> __attribute__((alias("default_fex_custom_repack_postcall"))) void fex_custom_repack_postcall<&TestStruct1::Next>(const const_void_ptr&);
template<> __attribute__((alias("default_fex_custom_repack_postcall"))) void fex_custom_repack_postcall<&TestStruct2::Next>(const const_void_ptr&);

void fexfn_impl_libfex_thunk_test_TestFunction(TestStruct1* arg) {
  fprintf(stderr, "Hello from %s\n", __FUNCTION__);
  fprintf(stderr, "  TestStruct1: %c %d %p\n", arg->Data2, arg->Data1, arg->Next);
  if (!arg->Next || ((TestBaseStruct*)arg->Next)->Type != StructType::Struct2) {
    fprintf(stderr, "ERROR: Expected Next with StructType::Struct2, got %d\n", (arg->Next ? (int)((TestBaseStruct*)arg->Next)->Type : 0));
    std::abort();
  }
  auto Next = (TestStruct2*)arg->Next;
  fprintf(stderr, "  TestStruct2: %d %p\n", Next->Data1, Next->Next);
  auto Next2 = (TestStruct1*)Next->Next;
  fprintf(stderr, "  TestStruct3: %d %p\n", Next2->Data1, Next2->Next);
}

EXPORTS(libfex_thunk_test)
