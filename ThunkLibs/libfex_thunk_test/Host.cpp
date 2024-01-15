/*
$info$
tags: thunklibs|fex_thunk_test
$end_info$
*/

#include <cstddef>
#include <dlfcn.h>

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

EXPORTS(libfex_thunk_test)
