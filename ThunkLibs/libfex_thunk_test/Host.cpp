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

EXPORTS(libfex_thunk_test)
