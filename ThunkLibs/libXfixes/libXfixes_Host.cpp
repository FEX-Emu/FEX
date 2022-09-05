/*
$info$
tags: thunklibs|X11
$end_info$
*/

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>

#include "common/Host.h"
#include <dlfcn.h>

#include "thunkgen_host_libXfixes.inl"

EXPORTS(libXfixes)
