/*
$info$
tags: thunklibs|asound
$end_info$
*/

extern "C" {
#include <alsa/asoundlib.h>
}

#include <stdio.h>
#include <cstring>
#include <map>
#include <string>

#include "common/Guest.h"
#include <stdarg.h>

#include "thunkgen_guest_libasound.inl"

LOAD_LIB(libasound)
