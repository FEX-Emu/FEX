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

#include "thunks.inl"
#include "function_packs.inl"
#include "function_packs_public.inl"

LOAD_LIB(libxshmfence)
