#include <stdio.h>

#include <alsa/asoundlib.h>

#include "../Thunk.h"
#include <dlfcn.h>

#include "libasound_Forwards.inl"

static ExportEntry exports[] = {
    #include "libasound_Thunkmap.inl"
    { nullptr, nullptr }
};

EXPORTS(libasound) 

