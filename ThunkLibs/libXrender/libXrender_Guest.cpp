/*
$info$
tags: thunklibs|X11
$end_info$
*/

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>

#include "common/Guest.h"

#include "thunkgen_guest_libXrender.inl"

LOAD_LIB(libXrender)
