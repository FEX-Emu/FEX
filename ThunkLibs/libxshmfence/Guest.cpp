/*
$info$
tags: thunklibs|xshmfence
$end_info$
*/

extern "C" {
#include <X11/xshmfence.h>
}

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "common/Guest.h"
#include <stdarg.h>

#include "thunkgen_guest_libxshmfence.inl"

LOAD_LIB(libxshmfence)
