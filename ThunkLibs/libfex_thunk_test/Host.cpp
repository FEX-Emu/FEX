/*
$info$
tags: thunklibs|fex_thunk_test
$end_info$
*/

#include <dlfcn.h>

#include <unordered_map>

#include "common/Host.h"

#include "api.h"

#include "thunkgen_host_libfex_thunk_test.inl"

static uint32_t fexfn_impl_libfex_thunk_test_QueryOffsetOf(guest_layout<ReorderingType*> data, int index) {
    if (index == 0) {
        return offsetof(guest_layout<ReorderingType>::type, a);
    } else {
        return offsetof(guest_layout<ReorderingType>::type, b);
    }
}

void fex_custom_repack_entry(host_layout<CustomRepackedType>& to, guest_layout<CustomRepackedType> const& from) {
  to.data.custom_repack_invoked = 1;
}

bool fex_custom_repack_exit(guest_layout<CustomRepackedType>& to, host_layout<CustomRepackedType> const& from) {
  return false;
}

template<StructType TypeIndex, typename Type>
static const TestBaseStruct* convert(const TestBaseStruct* source) {
    // Using malloc here since no easily available type information is available at the time of destruction.
    auto guest_next = reinterpret_cast<guest_layout<Type>*>((void*)source);
    auto child_mem = (char*)aligned_alloc(alignof(host_layout<Type>), sizeof(host_layout<Type>));
    auto child = new (child_mem) host_layout<Type> { *guest_next };

    fex_custom_repack_entry(*child, *reinterpret_cast<guest_layout<Type>*>((void*)(source)));

    return (const TestBaseStruct*)child;
}

template<StructType TypeIndex, typename Type>
static void convert_to_guest(void* into, const TestBaseStruct* from) {
    auto typed_into = (guest_layout<Type>*)into;
    auto oldNext = typed_into->data.Next;
    *typed_into = to_guest(to_host_layout(*(Type*)from));
    typed_into->data.Next = oldNext;

    fex_custom_repack_exit(*typed_into, to_host_layout(*(Type*)from));
}

template<StructType TypeIndex, typename Type>
inline constexpr std::pair<StructType, std::pair<const TestBaseStruct*(*)(const TestBaseStruct*), void(*)(void*, const TestBaseStruct*)>> converters =
  { TypeIndex, { convert<TypeIndex, Type>, convert_to_guest<TypeIndex, Type> } };

static std::unordered_map<StructType, std::pair<const TestBaseStruct*(*)(const TestBaseStruct*), void(*)(void*, const TestBaseStruct*)>> next_handlers {
    converters<StructType::Struct1, TestStruct1>,
    converters<StructType::Struct2, TestStruct2>,
};

static void default_fex_custom_repack_entry(TestBaseStruct& into, const guest_layout<TestBaseStruct>* from) {
  if (!from->data.Next.get_pointer()) {
      into.Next = nullptr;
      return;
  }
  auto typed_source = reinterpret_cast<const guest_layout<TestBaseStruct>*>(from->data.Next.get_pointer());

  auto next_handler = next_handlers.at(StructType { typed_source->data.Type.data });

  into.Next = (TestBaseStruct*)next_handler.first((const TestBaseStruct*)typed_source);
}

static void default_fex_custom_repack_reverse(guest_layout<TestBaseStruct>& into, const TestBaseStruct* from) {
  auto NextHost = from->Next;
  if (!NextHost) {
    return;
  }

  auto next_handler = next_handlers.at(static_cast<StructType>(into.data.Next.get_pointer()->data.Type.data));
  next_handler.second((void*)into.data.Next.get_pointer(), from->Next);

  free((void*)NextHost);
}

#define CREATE_INFO_DEFAULT_CUSTOM_REPACK(name) \
  void fex_custom_repack_entry(host_layout<name>& into, const guest_layout<name>& from) { \
    default_fex_custom_repack_entry(*(TestBaseStruct*)&into.data, reinterpret_cast<const guest_layout<TestBaseStruct>*>(&from)); \
  } \
   \
  bool fex_custom_repack_exit(guest_layout<name>& into, const host_layout<name>& from) { \
    auto prev_next = into.data.Next; \
    default_fex_custom_repack_reverse(*reinterpret_cast<guest_layout<TestBaseStruct>*>(&into), &reinterpret_cast<const TestBaseStruct&>(from.data)); \
    into = to_guest(from); \
    into.data.Next = prev_next; \
    return true; \
  }

CREATE_INFO_DEFAULT_CUSTOM_REPACK(TestStruct1)
CREATE_INFO_DEFAULT_CUSTOM_REPACK(TestStruct2)

void fex_custom_repack_entry(host_layout<TestBaseStruct>&, const guest_layout<TestBaseStruct>&) {
  std::abort();
}

bool fex_custom_repack_exit(guest_layout<TestBaseStruct>&, const host_layout<TestBaseStruct>&) {
  std::abort();
  return false;
}

EXPORTS(libfex_thunk_test)
