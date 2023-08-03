/*
$info$
tags: thunklibs|X11
$end_info$
*/

#include <stdio.h>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/Xregion.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
}
#undef min
#undef max

#include "common/Host.h"
#include <dlfcn.h>

#include "thunkgen_host_libXrender.inl"

EXPORTS(libXrender)
